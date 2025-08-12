#include <iostream>

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

    case AST_VARIABLE:
        print_variable((Variable*) node);
        break;

    case AST_TYPE_PRIMITIVE:
        print_primitive_type((PrimitiveType*) node);
        break;

    case AST_TYPE_INDIRECTION:
        print_indirection_type((IndirectionType*) node);
        break;

    case AST_ASSIGNMENT:
    case AST_PLUS:
    case AST_MINUS:
        print_binop((BinaryOperator*) node);
        break;

    case AST_LITERAL:
        print_literal((Literal*) node);
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

    for (auto v : node->get_variables()) {
        print(v, true);
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

void PrettyPrinter::print_variable(Variable* node) {
    if (node->is_const()) {
        print("const ");
    } else {
        print("let ");
    }

    print(node->get_name());

    if (node->get_type()) {
        print(" : ");
        print(node->get_type());
    }

    if (node->get_expression()) {
        print(" = ");
        print(node->get_expression());
    }
}

void PrettyPrinter::print_primitive_type(PrimitiveType* node) {
    print(node->get_token());
}

void PrettyPrinter::print_indirection_type(IndirectionType* node) {
    print(node->get_subtype());
    print(node->get_token());
}

void PrettyPrinter::print_binop(BinaryOperator* node) {
    print(node->get_left());
    print(node->get_token());
    print(node->get_right());
}

void PrettyPrinter::print_literal(Literal* node) {
    bool single_quoted = false;
    bool quoted = false;

    auto kind = node->get_token().get_kind();

    if (kind == TK_LITERAL_SINGLE_QUOTE_STRING) {
        single_quoted = true;
    }

    if (kind == TK_LITERAL_CHAR) {
        single_quoted = true;
    }

    if (kind == TK_LITERAL_DOUBLE_QUOTE_STRING) {
        quoted = true;
    }

    if (single_quoted) {
        print("'");
    } else if (quoted) {
        print("\"");
    }

    print(node->get_token());

    if (single_quoted) {
        print("'");
    } else if (quoted) {
        print("\"");
    }
}

void PrettyPrinter::print(const Token& token) {
    if (token.get_value() != nullptr) {
        out << token.get_value();
    }
}

std::string PrettyPrinter::get_output() {
    return out.str();
}

void PrettyPrinter::print(const std::string& msg) {
    out << msg;
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
