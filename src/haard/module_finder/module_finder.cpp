#include <haard/module_finder/module_finder.h>
#include <algorithm>
#include <fstream>

using namespace haard;

static std::string trim(const std::string& text) {
    size_t first = text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

ModuleFinder::ModuleFinder() {
    block = INVALID_ROOT;
}

// Two passes, and the second one is not an optimization: a dependency line
// names a path whose block may come later in the file, and two roots are
// allowed to see each other -- record 0007 lets modules import each other, so
// forbidding it between roots would be an inconsistency nothing asked for.
//
// The second pass turns each dependency's path into a block index, which is
// what makes a dependency pointing at no block an error for free. It has to be
// one: if png depends on ../sys/zip/1.0 and that directory has no block of its
// own, zip's own internal imports would have nowhere to resolve.
bool ModuleFinder::load(const std::filesystem::path& table) {
    std::ifstream input(table);

    roots.clear();
    error.clear();
    block = INVALID_ROOT;
    table_name = table.string();

    if (!input.is_open()) {
        error = table_name + ": cannot open the roots table";
        return false;
    }

    // a relative path in the table is relative to the table, never to the
    // working directory, or the same command run from two places resolves to
    // two different files and the cache key moves with it
    std::filesystem::path directory = table.parent_path();
    std::vector<Dependency> pending;
    std::string line;
    u32 number = 0;

    while (std::getline(input, line)) {
        number++;

        std::string text = trim(line);

        // A whole line and never a trailing one: a path may hold a '#' and
        // the generated file is worth a header saying it is generated. The
        // example's own table opens with one
        if (text.size() == 0 || text[0] == '#') {
            continue;
        }

        // indentation is the whole syntax: at column zero a block opens, and
        // anything indented belongs to the block above it
        if (line[0] == ' ' || line[0] == '\t') {
            if (!read_dependency(text, number, directory, pending)) {
                return false;
            }
        } else if (!read_header(text, number, directory)) {
            return false;
        }
    }

    for (const Dependency& dependency : pending) {
        u32 target = block_with_path(dependency.path);

        if (target == INVALID_ROOT) {
            return fail(dependency.line, "'" + dependency.text +
                        "' names a directory that has no root block");
        }

        if (!add_visible(dependency.block, dependency.name, target,
                         dependency.line)) {
            return false;
        }
    }

    return true;
}

const std::string& ModuleFinder::get_error() {
    return error;
}

u32 ModuleFinder::root_of_file(const std::filesystem::path& file) {
    std::filesystem::path target = normalize(file);
    u32 best = INVALID_ROOT;
    size_t length = 0;

    for (size_t i = 0; i < roots.size(); i++) {
        if (!starts_with(target, roots[i].path)) {
            continue;
        }

        // the longest and not the first: roots nest in practice, and a root
        // at sys/ must not capture the files of the root at sys/zip/1.0
        size_t size = std::distance(roots[i].path.begin(), roots[i].path.end());

        if (best == INVALID_ROOT || size > length) {
            best = (u32) i;
            length = size;
        }
    }

    return best;
}

// The name always has a segment: an import that failed to parse logged an
// error and the phase gate stopped the compilation before anything got here
FindResult ModuleFinder::find(u32 root, const std::string& name) {
    FindResult result;
    std::vector<std::string> segments = split_name(name);
    u32 target = target_of(root, segments);

    result.root = target;

    if (target == INVALID_ROOT) {
        result.status = FIND_NO_ROOT;
        return result;
    }

    std::filesystem::path path = roots[target].path;

    if (segments.size() == 1) {
        // 'import foobar' is <foobar's root>/foobar.hd: with one segment the
        // name does double duty, naming the library and the file inside it
        path /= segments[0];
    } else {
        for (size_t i = 1; i < segments.size(); i++) {
            path /= segments[i];
        }
    }

    path += ".hd";

    result.status = std::filesystem::is_regular_file(path) ? FIND_OK
                                                           : FIND_NO_FILE;
    result.path = path;

    if (result.status != FIND_OK) {
        result.root = INVALID_ROOT;
    }

    return result;
}

FindResult ModuleFinder::find_all(u32 root, const std::string& name) {
    FindResult result;
    std::vector<std::string> segments = split_name(name);
    u32 target = target_of(root, segments);

    result.root = target;

    if (target == INVALID_ROOT) {
        result.status = FIND_NO_ROOT;
        return result;
    }

    // and here the tail being empty means the root directory itself, where
    // for a plain import it meant a file named after the library. The star
    // says where to look, not what to look for, so the two rules part exactly
    // here and nowhere else
    std::filesystem::path path = roots[target].path;

    for (size_t i = 1; i < segments.size(); i++) {
        path /= segments[i];
    }

    result.path = path;

    if (!std::filesystem::is_directory(path)) {
        result.status = FIND_NO_FILE;
        result.root = INVALID_ROOT;
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".hd") {
            result.files.push_back(entry.path());
        }
    }

    // directory_iterator has no order of its own, and record 0006 hashes this
    // expansion. Sorting paths compares their bytes and asks no locale, so the
    // same directory gives the same list on every machine
    std::sort(result.files.begin(), result.files.end());
    result.status = FIND_OK;

    return result;
}

std::string ModuleFinder::module_name_of_file(u32 root,
                                              const std::filesystem::path& file) {
    std::filesystem::path relative =
        std::filesystem::relative(normalize(file), roots[root].path);

    relative.replace_extension();

    // 'import foobar' and 'import foobar.foobar' name the same file, since one
    // segment does double duty. The shorter spelling is the module's name, so
    // that the entry file of a library is called after the library
    if (relative == std::filesystem::path(roots[root].name)) {
        return roots[root].name;
    }

    std::string name = roots[root].name;

    for (const auto& segment : relative) {
        name += '.';
        name += segment.string();
    }

    return name;
}

u32 ModuleFinder::get_root_count() {
    return (u32) roots.size();
}

const std::string& ModuleFinder::get_root_name(u32 root) {
    return roots[root].name;
}

const std::filesystem::path& ModuleFinder::get_root_path(u32 root) {
    return roots[root].path;
}

// 'root <path> = <name>'. The name is taken from the last '=' so that a path
// holding one still opens its block
bool ModuleFinder::read_header(const std::string& text, u32 line,
                               const std::filesystem::path& directory) {
    if (text.rfind("root", 0) != 0 || text.size() < 5 ||
        (text[4] != ' ' && text[4] != '\t')) {
        return fail(line, "expected a root block or an indented dependency");
    }

    std::string rest = trim(text.substr(4));
    size_t equals = rest.rfind('=');

    if (equals == std::string::npos) {
        return fail(line, "a root block needs '= <name>'");
    }

    std::string text_path = trim(rest.substr(0, equals));
    std::string name = trim(rest.substr(equals + 1));

    if (text_path.size() == 0 || name.size() == 0) {
        return fail(line, "a root block is 'root <path> = <name>'");
    }

    std::filesystem::path path = normalize(directory / text_path);
    u32 other = block_with_path(path);

    if (other != INVALID_ROOT) {
        return fail(line, "'" + text_path + "' is the directory of the root '" +
                    roots[other].name + "' above");
    }

    Root root;

    root.name = name;
    root.path = path;

    roots.push_back(root);
    block = (u32) roots.size() - 1;

    return add_visible(block, name, block, line);
}

// '<name> -> <path>'. The path is not looked up here: its block may still be
// further down the file
bool ModuleFinder::read_dependency(const std::string& text, u32 line,
                                   const std::filesystem::path& directory,
                                   std::vector<Dependency>& pending) {
    if (block == INVALID_ROOT) {
        return fail(line, "a dependency before any root block");
    }

    size_t arrow = text.find("->");

    if (arrow == std::string::npos) {
        return fail(line, "a dependency is '<name> -> <path>'");
    }

    Dependency dependency;

    dependency.block = block;
    dependency.name = trim(text.substr(0, arrow));
    dependency.text = text;
    dependency.path = normalize(directory / trim(text.substr(arrow + 2)));
    dependency.line = line;

    if (dependency.name.size() == 0 || text.substr(arrow + 2).size() == 0) {
        return fail(line, "a dependency is '<name> -> <path>'");
    }

    pending.push_back(dependency);

    return true;
}

bool ModuleFinder::add_visible(u32 block, const std::string& name, u32 target,
                               u32 line) {
    if (visible_root(block, name) != INVALID_ROOT) {
        return fail(line, "the root '" + roots[block].name + "' sees '" + name +
                    "' twice");
    }

    roots[block].visible_names.push_back(name);
    roots[block].visible_roots.push_back(target);

    return true;
}

// a half read table must not be usable: a missing block would look like a
// missing entry to every caller after it
bool ModuleFinder::fail(u32 line, const std::string& message) {
    error = table_name + ":" + std::to_string(line) + ": " + message;
    roots.clear();

    return false;
}

u32 ModuleFinder::visible_root(u32 root, const std::string& name) {
    for (size_t i = 0; i < roots[root].visible_names.size(); i++) {
        if (roots[root].visible_names[i] == name) {
            return roots[root].visible_roots[i];
        }
    }

    return INVALID_ROOT;
}

u32 ModuleFinder::block_with_path(const std::filesystem::path& path) {
    for (size_t i = 0; i < roots.size(); i++) {
        if (roots[i].path == path) {
            return (u32) i;
        }
    }

    return INVALID_ROOT;
}

u32 ModuleFinder::target_of(u32 root, const std::vector<std::string>& segments) {
    if (root == INVALID_ROOT) {
        return INVALID_ROOT;
    }

    return visible_root(root, segments[0]);
}

std::vector<std::string> ModuleFinder::split_name(const std::string& name) {
    std::vector<std::string> segments;
    size_t start = 0;
    size_t dot = name.find('.');

    while (dot != std::string::npos) {
        segments.push_back(name.substr(start, dot - start));
        start = dot + 1;
        dot = name.find('.', start);
    }

    segments.push_back(name.substr(start));

    return segments;
}

// weakly_canonical and not canonical: the table is read before any of these
// directories is opened, and canonical throws on one that is not there.
// It does not fold case either, which is what the all-lowercase convention
// wants -- breaking the convention fails loudly on Linux instead of meaning
// two things on macOS
std::filesystem::path ModuleFinder::normalize(const std::filesystem::path& path) {
    std::filesystem::path result = std::filesystem::weakly_canonical(path);

    // a trailing separator in the table leaves a dot element behind, and a dot
    // element matches no component of a real file
    if (result.filename() == ".") {
        result = result.parent_path();
    }

    return result;
}

bool ModuleFinder::starts_with(const std::filesystem::path& file,
                               const std::filesystem::path& root) {
    auto left = file.begin();
    auto right = root.begin();

    while (right != root.end()) {
        if (left == file.end() || *left != *right) {
            return false;
        }

        left++;
        right++;
    }

    return true;
}
