#include <iostream>
#include <haard/pretty_printer/pretty_printer.h>

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    context = nullptr;
    ast = nullptr;
}

bool PrettyPrinter::print(std::ostream& out) {
    u32 root = ast->get_root();

    if (root == 0) {
        return false;
    }

    output.str("");
    print_node(root);
    out << output.str() << '\n';

    return true;
}

void PrettyPrinter::print_node(u32 node) {
    if (node == 0) {
        return;
    }

    auto kind = ast->get_node(node)->get_kind();

    switch (kind) {
        case AST_MODULE:
            print_module(node);
            break;

        case AST_IMPORT:
            print_import(node);
            break;

        case AST_IMPORT_PATH:
            print_import_path(node);
            break;

        case AST_IMPORT_PATH_SEGMENT:
            print_import_path_segment(node);
            break;

        case AST_IMPORT_ALIAS:
            print_import_alias(node);
            break;

        case AST_LET_DECLARATION:
            print_let_declaration(node);
            break;

        case AST_CONST_DECLARATION:
            print_const_declaration(node);
            break;

        case AST_PARAM:
            print_param(node);
            break;

        case AST_BINDING:
            print_binding(node);
            break;

        case AST_BINDING_NAME:
            print_binding_name(node);
            break;

        case AST_BINDING_TYPE:
            print_binding_type(node);
            break;

        case AST_BINDING_EXPRESSION:
            print_binding_expression(node);
            break;

        case AST_PLUS:
            print_plus_expression(node);
            break;

        case AST_MINUS:
            print_minus_expression(node);
            break;

        case AST_TIMES:
            print_times_expression(node);
            break;

        case AST_DIVISION:
            print_division_expression(node);
            break;

        case AST_MODULO:
            print_modulo_expression(node);
            break;

        case AST_SCOPE:
            print_scope(node);
            break;

        case AST_PARENTHESIS:
            print_parenthesis(node);
            break;

        case AST_IDENTIFIER:
            print_identifier(node);
            break;

        // one case per kind would be seven copies of the same line: a literal
        // is written back as the lexeme it was read from, whichever kind it is
        case AST_INTEGER_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
        case AST_CHAR_LITERAL:
        case AST_SYMBOL_LITERAL:
        case AST_TRUE:
        case AST_FALSE:
            print_literal(node);
            break;

        // a node kind with no case here would otherwise vanish from the
        // output without a trace, which is the worst way to find out that one
        // is missing
        default:
            output << "<no printer for ast kind " << (int) kind << ">";
            break;
    }
}

void PrettyPrinter::print_module(u32 node) {
    print_children_joined(node, "\n");
}

void PrettyPrinter::print_import(u32 node) {
    output << "import ";
    print_children(node);
}

void PrettyPrinter::print_import_path(u32 node) {
    print_children_joined(node, ".");
}

void PrettyPrinter::print_import_path_segment(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_import_alias(u32 node) {
    print_string(" as ");
    print_node_token(node);
}

void PrettyPrinter::print_let_declaration(u32 node) {
    print_string("let ");
    print_children(node);
}

void PrettyPrinter::print_const_declaration(u32 node) {
    print_string("const ");
    print_children(node);
}

void PrettyPrinter::print_param(u32 node) {
    print_string("@");
    print_children(node);
}

void PrettyPrinter::print_binding(u32 node) {
    print_children(node);
}

void PrettyPrinter::print_binding_name(u32 node) {
    print_children(node);
}

void PrettyPrinter::print_binding_type(u32 node) {
    print_string(" : ");
    print_children(node);
}

void PrettyPrinter::print_binding_expression(u32 node) {
    print_string(" = ");
    print_children(node);
}

void PrettyPrinter::print_plus_expression(u32 node) {
    print_children_joined(node, " + ");
}

void PrettyPrinter::print_minus_expression(u32 node) {
    print_children_joined(node, " - ");
}

void PrettyPrinter::print_times_expression(u32 node) {
    print_children_joined(node, " * ");
}

void PrettyPrinter::print_division_expression(u32 node) {
    print_children_joined(node, " / ");
}

void PrettyPrinter::print_modulo_expression(u32 node) {
    print_children_joined(node, " % ");
}

// the two forms are told apart by the number of children, because the builder
// writes the '::name' form with no alias: one child is '::name', two are
// 'alias::name'. The '::' is printed glued, the way the '.' of an import path
// is — it resolves a name, it is not an operator between operands
void PrettyPrinter::print_scope(u32 node) {
    u32 child = ast->get_node(node)->get_children();

    if (child != 0 && ast->get_node(child)->get_sibling() == 0) {
        print_string("::");
        print_node(child);
        return;
    }

    print_children_joined(node, "::");
}

// the parentheses are a node, so printing them is a walk like everything else:
// the printer never asks what binds tighter than what. The grouping the source
// had is in the tree, and this writes it back
void PrettyPrinter::print_parenthesis(u32 node) {
    print_string("(");
    print_children(node);
    print_string(")");
}

void PrettyPrinter::print_identifier(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_literal(u32 node) {
    print_node_token(node);
}

void PrettyPrinter::print_children(u32 node) {
    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        print_node(child);
        child = ast->get_node(child)->get_sibling();
    }
}

void PrettyPrinter::print_children_joined(u32 node, const std::string& sep) {
    bool first = true;
    u32 child = ast->get_node(node)->get_children();

    while (child != 0) {
        if (!first) {
            print_string(sep);
        }

        print_node(child);
        child = ast->get_node(child)->get_sibling();
        first = false;
    }
}

void PrettyPrinter::print_node_token(u32 node) {
    print_token(ast->get_node(node)->get_token());
}

void PrettyPrinter::print_token(u32 token) {
    print_string(context->get_token_value(token));
}

void PrettyPrinter::print_string(const std::string_view& s) {
    output << s;
}


void PrettyPrinter::set_context(Context* context) {
    this->context = context;
    this->ast = context == nullptr ? nullptr : context->get_ast();
}
