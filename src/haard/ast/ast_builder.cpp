#include <haard/ast/ast_builder.h>

using namespace haard;

AstBuilder::AstBuilder() {
    ast = nullptr;
}

void AstBuilder::set_ast(Ast* ast) {
    this->ast = ast;
}

// appends a child and gives back the new last one. A 'last' of 0 means the
// caller does not know it, not that the parent has no children yet: passing 0
// every time still builds the right list, it only makes each append walk to the
// end. Forgetting to carry the result of this call is then slow, never wrong
u32 AstBuilder::add_child(u32 parent, u32 last, u32 child) {
    if (child == 0) {
        return last;
    }

    if (last == 0) {
        add_last_child(parent, child);
    } else {
        ast->get_node(last)->set_sibling(child);
    }

    return child;
}

// a module is the root of a haard tree by definition, so building one is what
// tells the Ast where a walk starts
u32 AstBuilder::make_module() {
    u32 node = make_node(AST_MODULE, 0);

    ast->set_root(node);

    return node;
}

u32 AstBuilder::make_import(u32 token, u32 path, u32 alias) {
    u32 node = make_node(AST_IMPORT, token);

    u32 last = add_child(node, 0, path);

    add_child(node, last, alias);

    return node;
}

u32 AstBuilder::make_import_path() {
    return make_node(AST_IMPORT_PATH, 0);
}

u32 AstBuilder::make_import_path_segment(u32 token) {
    return make_node(AST_IMPORT_PATH_SEGMENT, token);
}

u32 AstBuilder::make_import_alias(u32 token) {
    return make_node(AST_IMPORT_ALIAS, token);
}

u32 AstBuilder::make_let_declaration(u32 token, u32 binding) {
    u32 node = make_node(AST_LET_DECLARATION, token);

    add_child(node, 0, binding);

    return node;
}

u32 AstBuilder::make_const_declaration(u32 token, u32 binding) {
    u32 node = make_node(AST_CONST_DECLARATION, token);

    add_child(node, 0, binding);

    return node;
}

u32 AstBuilder::make_param(u32 token, u32 name, u32 type) {
    u32 node = make_node(AST_PARAM, token);

    u32 last = add_child(node, 0, name);

    add_child(node, last, type);

    return node;
}

u32 AstBuilder::make_binding(u32 name, u32 type, u32 expression) {
    u32 node = make_node(AST_BINDING, 0);

    u32 last = add_child(node, 0, name);

    last = add_child(node, last, type);
    add_child(node, last, expression);

    return node;
}

u32 AstBuilder::make_binding_name(u32 child) {
    u32 node = make_node(AST_BINDING_NAME, 0);

    add_child(node, 0, child);

    return node;
}

u32 AstBuilder::make_binding_type(u32 child) {
    u32 node = make_node(AST_BINDING_TYPE, 0);

    add_child(node, 0, child);

    return node;
}

u32 AstBuilder::make_binding_expression(u32 child) {
    u32 node = make_node(AST_BINDING_EXPRESSION, 0);

    add_child(node, 0, child);

    return node;
}

u32 AstBuilder::make_binary_operator(AstNodeKind kind, u32 token, u32 left,
                                     u32 right) {
    u32 node = make_node(kind, token);

    u32 last = add_child(node, 0, left);

    add_child(node, last, right);

    return node;
}

// '::name' is written with a 0 alias, so the node is left with a single child.
// That is what tells the two forms apart later: one child is '::name', two are
// 'alias::name'
u32 AstBuilder::make_scope(u32 token, u32 alias, u32 name) {
    u32 node = make_node(AST_SCOPE, token);

    u32 last = add_child(node, 0, alias);

    add_child(node, last, name);

    return node;
}

u32 AstBuilder::make_parenthesis(u32 token, u32 expression) {
    u32 node = make_node(AST_PARENTHESIS, token);

    add_child(node, 0, expression);

    return node;
}

u32 AstBuilder::make_identifier(u32 token) {
    return make_node(AST_IDENTIFIER, token);
}

u32 AstBuilder::make_node(AstNodeKind kind, u32 token) {
    AstNode node;

    node.set_kind(kind);
    node.set_token(token);

    return ast->push(node);
}

// the safety net for a caller that did not carry the last child: it looks the
// end of the list up. Costs one walk, which is why add_child gives the last
// child back instead of relying on this
void AstBuilder::add_last_child(u32 parent, u32 child) {
    u32 last = last_child_of(parent);

    if (last == 0) {
        ast->get_node(parent)->set_children(child);
        return;
    }

    ast->get_node(last)->set_sibling(child);
}

u32 AstBuilder::last_child_of(u32 parent) {
    u32 last = ast->get_node(parent)->get_children();

    if (last == 0) {
        return 0;
    }

    while (ast->get_node(last)->get_sibling() != 0) {
        last = ast->get_node(last)->get_sibling();
    }

    return last;
}
