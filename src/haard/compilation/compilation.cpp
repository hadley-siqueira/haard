#include <haard/compilation/compilation.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <stdexcept>

using namespace haard;

Compilation::Compilation() {
    has_table = false;
}

Compilation::~Compilation() {
    for (Module* module : modules) {
        delete module;
    }
}

bool Compilation::set_roots(const std::filesystem::path& table) {
    has_table = finder.load(table);

    if (!has_table) {
        error = finder.get_error();
    }

    return has_table;
}

const std::string& Compilation::get_error() {
    return error;
}

bool Compilation::build(const std::filesystem::path& entry) {
    u32 root = INVALID_ROOT;

    if (has_table) {
        root = finder.root_of_file(entry);

        if (root == INVALID_ROOT) {
            error = entry.string() + ": no root block of the table covers this "
                    "file";
            return false;
        }
    }

    intern(entry, root);

    // the list grows while it is walked, and that is the loop: everything an
    // import reached is appended and will be visited in its turn
    for (u32 i = 0; i < modules.size(); i++) {
        load(i);

        // a module that did not parse has no tree to ask, and the phase gate
        // every reader of an Ast relies on says so. Its diagnostics are kept
        // and the other modules still load
        if (modules[i]->is_parsed()) {
            resolve_imports(i);
        }
    }

    return !has_errors();
}

u32 Compilation::get_module_count() {
    return (u32) modules.size();
}

Module* Compilation::get_module(u32 index) {
    return modules[index];
}

bool Compilation::has_errors() {
    if (error.size() > 0) {
        return true;
    }

    for (Module* module : modules) {
        if (module->get_logger()->has_errors()) {
            return true;
        }
    }

    return false;
}

void Compilation::print_diagnostics(std::ostream& out) {
    for (Module* module : modules) {
        module->get_logger()->print(out);
    }
}

u32 Compilation::intern(const std::filesystem::path& path, u32 root) {
    // The key is canonical and the path kept is the one that was passed. They
    // differ for the entry file, which arrives spelled the way the command
    // line spelled it: without this, an entry that some module also imports
    // would be interned twice under two spellings and compiled twice. The key
    // is an identity -- it is what agenda 3.4 will hash -- and the path is
    // only what gets opened
    std::string key = std::filesystem::weakly_canonical(path).string();
    auto found = by_path.find(key);

    if (found != by_path.end()) {
        return found->second;
    }

    Module* module = new Module();

    module->set_root(root);

    if (root != INVALID_ROOT) {
        module->set_name(finder.module_name_of_file(root, path));
    }

    modules.push_back(module);
    paths.push_back(path);
    by_path[key] = (u32) modules.size() - 1;

    return (u32) modules.size() - 1;
}

void Compilation::load(u32 index) {
    Scanner scanner;
    Parser parser;
    Module* module = modules[index];

    scanner.set_module(module);

    try {
        scanner.get_tokens(paths[index]);
    } catch (const std::exception& failure) {
        // SourceFile::open throws when the file is missing or unreadable.
        // The finder checked that every imported file is there, so this is
        // either the entry file or a file that went away in between; neither
        // has an offset to point at, so neither is a diagnostic
        error = failure.what();
        return;
    }

    if (module->get_logger()->has_errors()) {
        return;
    }

    parser.set_module(module);
    parser.parse();

    module->set_parsed(!module->get_logger()->has_errors());
}

void Compilation::resolve_imports(u32 index) {
    Module* module = modules[index];
    u32 root = module->get_root();

    if (root == INVALID_ROOT) {
        return;
    }

    query.set_module(module);

    for (u32 import : query.get_imports()) {
        std::string name = query.get_import_name(import);
        bool star = query.is_star_import(import);
        FindResult result = star ? finder.find_all(root, name)
                                 : finder.find(root, name);

        if (result.status == FIND_NO_ROOT) {
            // the table is what is wrong, not the source, so the message says
            // which entry is missing from which block
            report(module, import, "the root '" + finder.get_root_name(root) +
                   "' has no entry named '" +
                   name.substr(0, name.find('.')) + "'");
            continue;
        }

        if (result.status == FIND_NO_FILE) {
            // and here the source is what is wrong, so the message shows the
            // path: with two versions of a library in one program, the author
            // cannot tell which one he reached without being told
            report(module, import, "there is no " +
                   std::string(star ? "directory " : "file ") +
                   result.path.string());
            continue;
        }

        // the imported file belongs to its OWN root and not to this one, or
        // zip 1.0's internal imports would resolve inside whoever imported it
        if (star) {
            for (const std::filesystem::path& file : result.files) {
                intern(file, result.root);
            }
        } else {
            intern(result.path, result.root);
        }
    }
}

void Compilation::report(Module* module, u32 import,
                         const std::string& message) {
    module->get_logger()->error(query.get_import_offset(import),
                                query.get_import_length(import), message);
}
