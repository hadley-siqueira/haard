// Runs the whole cycle over one project and writes down what it reached, so
// that the golden is the module graph:
//
//   build <case directory>
//
// The directory holds 'table.tbl' (or 'generated/table.tbl', or neither),
// 'entry' naming the file to start from, and the project's sources. For every
// module that was loaded the golden carries its name, its file, the imports
// its source wrote and what it declares -- the last one is the proof that the
// file was not merely found but scanned and parsed.
//
// Every path is printed relative to the case directory. An absolute one would
// carry this machine's home into the goldens.
#include <haard/ast_query/ast_query.h>
#include <haard/compilation/compilation.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

static std::filesystem::path base;

static std::string show(const std::filesystem::path& path) {
    if (path.empty()) {
        return "<no file>";
    }

    return std::filesystem::relative(path, base).string();
}

// One node per line, two spaces per level, kind and lexeme. The indentation is
// what records the shape, so comparing two of these as strings compares the two
// trees.
//
// The kind is a number and not a name on purpose: the named dump lives in
// tests/parser/parse_and_print.cpp behind a switch over every AstNodeKind, and
// copying it here would give the project two of them to keep in step. Nothing
// below is ever read unless the check fails, and when it does the answer is to
// run parse_and_print by hand on the file it names
static void dump_node(std::ostream& out, Module& module, u32 node, u32 depth) {
    while (node != 0) {
        AstNode* current = module.get_ast()->get_node(node);

        out << std::string(depth * 2, ' ') << (int) current->get_kind() << " '"
            << module.get_token_value(current->get_token()) << "'\n";

        dump_node(out, module, current->get_children(), depth + 1);
        node = current->get_sibling();
    }
}

static std::string dump_of(Module& module) {
    std::ostringstream out;

    dump_node(out, module, module.get_ast()->get_root(), 0);

    return out.str();
}

// The tree a module got through the import path, against the tree that same
// file gives when it is the only thing parsed.
//
// This is the one property a suite of single files cannot have: it is about
// what happens to the fifth file *because* four were parsed before it. The
// Compilation builds a Scanner and a Parser per module, and hoisting them out
// of the loop is the obvious optimization that would break this silently --
// every other golden here would still pass
static bool parsed_the_same_alone(Module* module) {
    Module alone;
    Scanner scanner;
    Parser parser;

    scanner.set_module(&alone);
    scanner.get_tokens(module->get_source_file()->get_path());

    if (alone.get_logger()->has_errors()) {
        return false;
    }

    parser.set_module(&alone);
    parser.parse();

    return dump_of(alone) == dump_of(*module);
}

static void print_declarations(AstQuery& query, std::vector<u32> nodes) {
    for (u32 node : nodes) {
        std::cout << "        declares " << query.get_declaration_name(node)
                  << '\n';
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: build <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "generated" / "table.tbl";
    Compilation compilation;

    base = std::filesystem::weakly_canonical(directory);

    if (!std::filesystem::exists(table)) {
        table = directory / "table.tbl";
    }

    // a case with no table at all is the single file compilation the driver
    // has always done, and its imports are not followed
    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);
    compilation.build(directory / entry);

    std::cout << "modules:\n";

    for (u32 i = 0; i < compilation.get_module_count(); i++) {
        Module* module = compilation.get_module(i);
        AstQuery query;

        std::cout << "    " << i << " "
                  << (module->get_name().size() > 0 ? module->get_name()
                                                    : "<unnamed>")
                  << "  " << show(module->get_source_file()->get_path())
                  << '\n';

        // errors are not the question -- an unresolved import is logged
        // against a module whose tree is perfectly good. The question is
        // whether there is a tree, which is the phase gate AstQuery relies on
        if (!module->is_parsed()) {
            std::cout << "        did not parse\n";
            continue;
        }

        query.set_module(module);

        for (u32 import : query.get_imports()) {
            std::cout << "        imports " << query.get_import_name(import)
                      << (query.is_star_import(import) ? ".*" : "") << '\n';
        }

        print_declarations(query, query.get_functions());
        print_declarations(query, query.get_classes());
        print_declarations(query, query.get_structs());
        print_declarations(query, query.get_enums());
        print_declarations(query, query.get_unions());
    }

    if (compilation.get_error().size() > 0) {
        std::cout << "error: " << compilation.get_error() << '\n';
    }

    // every tree the loop built, against the same file parsed on its own
    for (u32 i = 0; i < compilation.get_module_count(); i++) {
        Module* module = compilation.get_module(i);

        if (module->is_parsed() && !parsed_the_same_alone(module)) {
            std::cout << "--- parsed alone: " << module->get_name()
                      << " differs from the tree the loop built\n";
        }
    }

    std::cout << "--- parsed alone: every module matches\n";

    {
        // an imported module knows itself by the canonical path the finder
        // resolved, so its diagnostics name this machine's home. That is
        // right for a user and wrong for a golden, so it is rewritten out
        std::ostringstream diagnostics;
        std::string text;
        // the entry module knows itself by the path the command line spelled
        // and an imported one by the canonical path the finder resolved, so
        // both spellings of this case's directory are rewritten out
        std::string prefixes[] = { base.string() + "/", directory.string() + "/" };

        compilation.print_diagnostics(diagnostics);
        text = diagnostics.str();

        for (const std::string& prefix : prefixes) {
            for (size_t at = text.find(prefix); at != std::string::npos;
                 at = text.find(prefix, at)) {
                text.erase(at, prefix.size());
            }
        }

        if (text.size() > 0) {
            std::cout << "diagnostics:\n" << text;
        }
    }

    return 0;
}
