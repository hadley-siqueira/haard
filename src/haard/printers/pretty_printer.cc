#include <haard/printers/pretty_printer.h>

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    indent_level = 0;
}

PrettyPrinter::~PrettyPrinter() {

}

void PrettyPrinter::print(Ast* node, bool newline) {
    if (node == nullptr) {
        return;
    }

    switch (node->get_kind()) {
    case AST_MODULE:
        print_module((Module*) node);
        break;

    case AST_IMPORT:
        print_import((Import*) node);
        break;
    }

    if (newline) {
        print_newline();
    }
}

void PrettyPrinter::print_module(Module* node) {
    for (auto i : node->get_imports()) {
        print(i, true);
    }
}

void PrettyPrinter::print_import(Import* node) {
    auto tokens = node->get_path();

    out << "import ";

    for (int i = 0; i < tokens.size(); ++i) {
        out << tokens[i].get_value();

        if (i > 0 && i < tokens.size() - 1) {
            out << ".";
        }
    }

    if (node->has_alias()) {
        out << " as ";
        print(node->get_alias());
    }
}

void PrettyPrinter::print(Token& token) {
    if (token.get_value() != nullptr) {
        out << token.get_value();
    }
}

void PrettyPrinter::print_newline() {
    out << '\n';
}

void PrettyPrinter::print_indentation() {
    for (int i = 0; i < indent_level; ++i) {
        out << "    ";
    }
}

void PrettyPrinter::indent() {
    indent_level++;
}

void PrettyPrinter::dedent() {
    if (indent_level > 0) {
        indent_level--;
    }
}
