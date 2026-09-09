#include <haard/manifest/manifest.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>

using namespace haard;

Manifest::Manifest() {
}

bool Manifest::load(const std::filesystem::path& file) {
    Scanner scanner;
    Parser parser;

    error.clear();
    name.clear();
    version.clear();
    dependencies.clear();
    prelude.clear();
    directory = file.parent_path();

    scanner.set_module(&module);

    try {
        scanner.get_tokens(file);
    } catch (const std::exception& failure) {
        // SourceFile::open throws when the file is missing or unreadable, and
        // a file that is not there has no offset to point at -- so this is a
        // message and not a diagnostic, the way a bad command line is
        error = failure.what();
        return false;
    }

    if (module.get_logger()->has_errors()) {
        return false;
    }

    parser.set_module(&module);

    u32 node = parser.parse_value();

    if (module.get_logger()->has_errors()) {
        return false;
    }

    read(node);

    return !module.get_logger()->has_errors();
}

// The top level, key by key. A manifest is a **record**: what it holds is
// written out, and a key nobody knows is a mistake rather than something to
// carry along -- a typo in a dependency name is the one thing a manifest must
// not swallow.
//
// 'tool' is the one key that is not read: it is reserved for whatever else
// wants to keep its settings beside the manifest, which is the answer
// pyproject.toml's '[tool.*]' gives to the junk drawer package.json became
void Manifest::read(u32 node) {
    if (node == 0) {
        return;
    }

    if (kind_of(node) != AST_HASH) {
        report(node, "a manifest is one record, and it opens with '{'");

        return;
    }

    for (u32 pair = first_child(node); pair != 0; pair = sibling_of(pair)) {
        if (kind_of(pair) != AST_HASH_PAIR) {
            continue;
        }

        std::string key = key_of(pair);
        u32 value = sibling_of(first_child(pair));

        if (key == "name") {
            name = text_of(value);
        } else if (key == "version") {
            version = text_of(value);
        } else if (key == "dependencies") {
            read_dependencies(value);
        } else if (key == "prelude") {
            read_prelude(value);
        } else if (key == "tool") {
            continue;
        } else {
            report(first_child(pair),
                   "'" + key + "' is not something a manifest holds");
        }
    }

    if (name.size() == 0) {
        report(node, "a manifest says what it is called: 'name'");
    }
}

// 'dependencies: [{name: "std", path: "../std"}]' -- a **list of records** and
// not a record keyed by name (form B, Hadley 2026-09-08). It costs a word per
// entry and buys the one thing the roots table exists for: two entries may
// carry the same name, which is one library at two versions in one program
void Manifest::read_dependencies(u32 list) {
    if (list == 0) {
        return;
    }

    if (kind_of(list) != AST_LIST) {
        report(list, "'dependencies' is a list, written with '[' and ']'");

        return;
    }

    for (u32 child = first_child(list); child != 0; child = sibling_of(child)) {
        read_requirement(child);
    }
}

void Manifest::read_requirement(u32 record) {
    if (kind_of(record) != AST_HASH) {
        report(record, "a dependency is a record: {name: \"std\", "
                       "path: \"../std\"}");

        return;
    }

    Requirement wanted;

    wanted.node = record;

    for (u32 pair = first_child(record); pair != 0; pair = sibling_of(pair)) {
        if (kind_of(pair) != AST_HASH_PAIR) {
            continue;
        }

        std::string key = key_of(pair);
        u32 value = sibling_of(first_child(pair));

        if (key == "name") {
            wanted.name = text_of(value);
        } else if (key == "path") {
            wanted.path = text_of(value);
        } else if (key == "version") {
            wanted.version = text_of(value);
        } else {
            report(first_child(pair),
                   "'" + key + "' is not something a dependency holds");
        }
    }

    if (wanted.name.size() == 0) {
        report(record, "a dependency says which name it is imported by");

        return;
    }

    // exactly one of the two, and said in those words: a dependency with
    // neither is not resolvable, and one with both does not say which wins
    if (wanted.path.size() == 0 && wanted.version.size() == 0) {
        report(record, "'" + wanted.name
                           + "' says neither a path nor a version");

        return;
    }

    if (wanted.path.size() > 0 && wanted.version.size() > 0) {
        report(record, "'" + wanted.name
                           + "' says both a path and a version, and only one "
                             "of them can be where it comes from");

        return;
    }

    dependencies.push_back(wanted);
}

void Manifest::read_prelude(u32 list) {
    if (list == 0) {
        return;
    }

    if (kind_of(list) != AST_LIST) {
        report(list, "'prelude' is a list, written with '[' and ']'");

        return;
    }

    for (u32 child = first_child(list); child != 0; child = sibling_of(child)) {
        std::string written = text_of(child);

        if (written.size() > 0) {
            prelude.push_back(written);
        }
    }
}

// The text a string literal holds, without the quotes it was written with.
// Either quote, since Haard takes both, and no escape is taken apart: a path
// and a module name hold none, and taking them apart here would be a second
// answer to a question the scanner already has one for
std::string Manifest::text_of(u32 node) {
    if (node == 0) {
        return "";
    }

    if (kind_of(node) != AST_STRING_LITERAL) {
        report(node, "this is written as text, in quotes");

        return "";
    }

    std::string written = std::string(
        module.get_token_value(module.get_ast()->get_node(node)->get_token()));

    if (written.size() >= 2) {
        return written.substr(1, written.size() - 2);
    }

    return "";
}

// The name a pair is keyed by. A key is an identifier and not a string: the
// UseResolver already refuses to look one up in a scope (record 0019's note
// on AST_HASH_PAIR), so it is a name in the record and nowhere else
std::string Manifest::key_of(u32 pair) {
    u32 key = first_child(pair);

    if (key == 0) {
        return "";
    }

    return std::string(
        module.get_token_value(module.get_ast()->get_node(key)->get_token()));
}

void Manifest::report(u32 node, const std::string& message) {
    Token& token = module.get_tokens()->get_token(
        module.get_ast()->get_node(node)->get_token());

    module.get_logger()->error(token.get_offset(), token.get_length(), message);
}

const std::string& Manifest::get_error() {
    return error;
}

Logger* Manifest::get_logger() {
    return module.get_logger();
}

const std::filesystem::path& Manifest::get_directory() {
    return directory;
}

const std::string& Manifest::get_name() {
    return name;
}

const std::string& Manifest::get_version() {
    return version;
}

const std::vector<Requirement>& Manifest::get_dependencies() {
    return dependencies;
}

const std::vector<std::string>& Manifest::get_prelude() {
    return prelude;
}

AstNodeKind Manifest::kind_of(u32 node) {
    return module.get_ast()->get_node(node)->get_kind();
}

u32 Manifest::first_child(u32 node) {
    return module.get_ast()->get_node(node)->get_children();
}

u32 Manifest::sibling_of(u32 node) {
    return module.get_ast()->get_node(node)->get_sibling();
}
