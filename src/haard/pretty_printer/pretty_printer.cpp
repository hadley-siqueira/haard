#include <iostream>
#include <haard/pretty_printer/pretty_printer.h>

using namespace haard;

PrettyPrinter::PrettyPrinter() {
    set_context(nullptr);
}

void PrettyPrinter::print() {
    print_node(1);
    std::cout << output.str() << std::endl;
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

        case AST_IDENTIFIER:
            print_identifier(node);
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

void PrettyPrinter::print_identifier(u32 node) {
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
    this->ast = context->get_ast();
}
