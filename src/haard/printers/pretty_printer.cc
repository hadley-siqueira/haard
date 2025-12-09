#include <haard/printers/pretty_printer.h>

using namespace haard;

void PrettyPrinter::print(Ast* node) {
    if (node == nullptr) {
        return;
    }

    auto kind = node->get_kind();

    switch (kind) {
        case AST_MODULE:
            print_module(node);
            break;

        default:
            print("UNKNOWN");
            break;
    }
}

void PrettyPrinter::print_module(Ast* node) {
    print_module_imports(node);
}

void PrettyPrinter::print_module_imports(Ast* node) {
    bool has_imports = false;

    for (auto child : node->get_children(AST_IMPORT)) {
        print(child);
        print("\n");
        has_imports = true;
    }

    if (has_imports) {
        print("\n");
    }
}

void PrettyPrinter::print_import(Ast* node) {
    print("import ");
    print(node->get_child(AST_IMPORT_PATH));
    print(node->get_child(AST_IMPORT_ALIAS));
}

void PrettyPrinter::print_import_path(Ast* node) {
    print_separated_by(node, ".");
}

void PrettyPrinter::print_import_alias(Ast* node) {
    print("as ");
    print(node->get_token());
}

void PrettyPrinter::print_identifier(Ast* node) {
    print(node->get_token());
}

void PrettyPrinter::print(const std::string& s) {
    out << s;
}

void PrettyPrinter::print(Token& token) {
    auto value = token.get_value();

    if (value) {
        out << value;
    }
}

void PrettyPrinter::print_separated_by(Ast* node, const std::string& s) {
    bool first = true;

    for (auto child : node->get_children()) {
        if (!first) {
            print(s);
        }

        print(child);
    }
}
