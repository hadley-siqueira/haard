// Dumps every answer AstQuery gives for one file, so that the golden reads as
// the list of what the module declares.
//
//   query <file.hd>
//
// A file that does not parse prints its diagnostics and nothing else: that is
// the phase gate AstQuery relies on, and the golden records it.
#include <haard/ast_query/ast_query.h>
#include <haard/module/module.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <iostream>

using namespace haard;

// a type declaration lists its members under it, indented, so the golden shows
// which body each method and field belongs to
static void print_names(const char* title, AstQuery& query,
                        std::vector<u32> nodes, bool with_members = false) {
    std::cout << title << ":\n";

    for (u32 node : nodes) {
        std::cout << "  " << query.get_declaration_name(node) << '\n';

        if (!with_members) {
            continue;
        }

        for (u32 member : query.get_members(node)) {
            std::cout << "    member " << query.get_declaration_name(member)
                      << '\n';
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: query <file.hd>\n";
        return 2;
    }

    Module module;
    Scanner scanner;
    Parser parser;
    AstQuery query;

    scanner.set_module(&module);
    parser.set_module(&module);
    query.set_module(&module);

    scanner.get_tokens(argv[1]);

    if (!module.get_logger()->has_errors()) {
        parser.parse();
    }

    if (module.get_logger()->has_errors()) {
        module.get_logger()->print(std::cout);
        return 0;
    }

    std::cout << "imports:\n";

    for (u32 import : query.get_imports()) {
        std::cout << "  " << query.get_import_name(import);

        if (query.is_star_import(import)) {
            std::cout << " (star)";
        }

        std::string alias = query.get_import_alias(import);

        if (alias.size() > 0) {
            std::cout << " as " << alias;
        }

        std::cout << '\n';
    }

    print_names("functions", query, query.get_functions());
    print_names("classes", query, query.get_classes(), true);
    print_names("structs", query, query.get_structs(), true);
    print_names("enums", query, query.get_enums(), true);
    print_names("unions", query, query.get_unions(), true);

    // a let or a const declares one name per binding, not one for the
    // statement, so the golden records the keyword of each statement it found
    std::cout << "global variables:\n";

    for (u32 variable : query.get_global_variables()) {
        AstNode* node = module.get_ast()->get_node(variable);

        std::cout << "  " << module.get_token_value(node->get_token());

        for (const std::string& name : query.get_binding_names(variable)) {
            std::cout << ' ' << name;
        }

        std::cout << '\n';
    }

    return 0;
}
