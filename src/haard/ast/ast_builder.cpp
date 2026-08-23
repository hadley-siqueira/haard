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

// the '*' that ends an import path, which takes everything under it
u32 AstBuilder::make_import_all(u32 token) {
    return make_node(AST_IMPORT_ALL, token);
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

u32 AstBuilder::make_type_declaration(AstNodeKind kind, u32 token) {
    return make_node(kind, token);
}

// the type between the brackets is the one being inherited from
u32 AstBuilder::make_super_type(u32 token, u32 type) {
    u32 node = make_node(AST_SUPER_TYPE, token);

    add_child(node, 0, type);

    return node;
}

u32 AstBuilder::make_type_body() {
    return make_node(AST_TYPE_BODY, 0);
}

// the same three parts a let binding has, under a kind of its own so that a
// walk can tell a field from a local without looking at its parent
u32 AstBuilder::make_field(u32 name, u32 type, u32 value) {
    u32 node = make_node(AST_FIELD, 0);

    u32 last = add_child(node, 0, name);

    last = add_child(node, last, type);
    add_child(node, last, value);

    return node;
}

u32 AstBuilder::make_function(u32 token) {
    return make_node(AST_FUNCTION, token);
}

u32 AstBuilder::make_generic_parameters(u32 token) {
    return make_node(AST_GENERIC_PARAMETERS, token);
}

u32 AstBuilder::make_function_return_type(u32 child) {
    u32 node = make_node(AST_FUNCTION_RETURN_TYPE, 0);

    add_child(node, 0, child);

    return node;
}

u32 AstBuilder::make_block() {
    return make_node(AST_BLOCK, 0);
}

u32 AstBuilder::make_pass(u32 token) {
    return make_node(AST_PASS, token);
}

u32 AstBuilder::make_if(u32 token) {
    return make_node(AST_IF, token);
}

u32 AstBuilder::make_elif(u32 token) {
    return make_node(AST_ELIF, token);
}

u32 AstBuilder::make_else(u32 token) {
    return make_node(AST_ELSE, token);
}

u32 AstBuilder::make_while(u32 token) {
    return make_node(AST_WHILE, token);
}

u32 AstBuilder::make_for(u32 token) {
    return make_node(AST_FOR, token);
}

u32 AstBuilder::make_for_each(u32 token) {
    return make_node(AST_FOR_EACH, token);
}

// the comma separated expressions between the 'for' and the first ';' or ':'.
// In the C shaped loop they are the initialisation; in the other two the last
// of them is the 'in' and the ones before it are what the sequence is taken
// apart into
u32 AstBuilder::make_for_head() {
    return make_node(AST_FOR_HEAD, 0);
}

u32 AstBuilder::make_for_condition() {
    return make_node(AST_FOR_CONDITION, 0);
}

u32 AstBuilder::make_for_increment() {
    return make_node(AST_FOR_INCREMENT, 0);
}

u32 AstBuilder::make_jump(AstNodeKind kind, u32 token, u32 expression) {
    u32 node = make_node(kind, token);

    add_child(node, 0, expression);

    return node;
}

u32 AstBuilder::make_label(u32 token, u32 name) {
    u32 node = make_node(AST_LABEL, token);

    add_child(node, 0, name);

    return node;
}

// the same three parts a field has, and for the same reason: the value is what
// the caller gets when it leaves the argument out
u32 AstBuilder::make_param(u32 token, u32 name, u32 type, u32 value) {
    u32 node = make_node(AST_PARAM, token);

    u32 last = add_child(node, 0, name);

    last = add_child(node, last, type);
    add_child(node, last, value);

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

u32 AstBuilder::make_index(u32 token, u32 target, u32 subscript) {
    u32 node = make_node(AST_INDEX, token);

    u32 last = add_child(node, 0, target);

    add_child(node, last, subscript);

    return node;
}

// the arguments are a node of their own rather than siblings of the callee, so
// that the callee is 'the first child' and nothing has to count
u32 AstBuilder::make_call(u32 token, u32 callee, u32 arguments) {
    u32 node = make_node(AST_CALL, token);

    u32 last = add_child(node, 0, callee);

    add_child(node, last, arguments);

    return node;
}

u32 AstBuilder::make_arguments(u32 token) {
    return make_node(AST_ARGUMENTS, token);
}

u32 AstBuilder::make_unary_operator(AstNodeKind kind, u32 token, u32 operand) {
    u32 node = make_node(kind, token);

    add_child(node, 0, operand);

    return node;
}

u32 AstBuilder::make_new(u32 token, u32 type, u32 arguments) {
    u32 node = make_node(AST_NEW, token);

    u32 last = add_child(node, 0, type);

    add_child(node, last, arguments);

    return node;
}

u32 AstBuilder::make_sizeof(u32 token, u32 expression) {
    u32 node = make_node(AST_SIZEOF, token);

    add_child(node, 0, expression);

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

u32 AstBuilder::make_literal(AstNodeKind kind, u32 token) {
    return make_node(kind, token);
}

// the name is a child rather than the node's token, because it may be a scope
// ('std::String') and that is already a node of its own
u32 AstBuilder::make_named_type(u32 name, u32 arguments) {
    u32 node = make_node(AST_NAMED_TYPE, 0);

    u32 last = add_child(node, 0, name);

    add_child(node, last, arguments);

    return node;
}

u32 AstBuilder::make_builtin_type(u32 token) {
    return make_node(AST_BUILTIN_TYPE, token);
}

u32 AstBuilder::make_generic_name(u32 name, u32 arguments) {
    u32 node = make_node(AST_GENERIC_NAME, 0);

    u32 last = add_child(node, 0, name);

    add_child(node, last, arguments);

    return node;
}

u32 AstBuilder::make_generic_arguments(u32 token) {
    return make_node(AST_GENERIC_ARGUMENTS, token);
}

u32 AstBuilder::make_pointer_type(u32 token, u32 type) {
    u32 node = make_node(AST_POINTER_TYPE, token);

    add_child(node, 0, type);

    return node;
}

u32 AstBuilder::make_reference_type(u32 token, u32 type) {
    u32 node = make_node(AST_REFERENCE_TYPE, token);

    add_child(node, 0, type);

    return node;
}

// the size is optional: 'T[]' is an array of no stated length
u32 AstBuilder::make_array_type(u32 token, u32 type, u32 size) {
    u32 node = make_node(AST_ARRAY_TYPE, token);

    u32 last = add_child(node, 0, type);

    add_child(node, last, size);

    return node;
}

u32 AstBuilder::make_list_type(u32 token, u32 type) {
    u32 node = make_node(AST_LIST_TYPE, token);

    add_child(node, 0, type);

    return node;
}

u32 AstBuilder::make_hash_type(u32 token, u32 key, u32 value) {
    u32 node = make_node(AST_HASH_TYPE, token);

    u32 last = add_child(node, 0, key);

    add_child(node, last, value);

    return node;
}

u32 AstBuilder::make_tuple_type(u32 token) {
    return make_node(AST_TUPLE_TYPE, token);
}

// the last child is the return type and everything before it is a parameter,
// which is what '(i32, i32) -> f64' means: one parameter, a tuple, and f64 back
u32 AstBuilder::make_function_type(u32 token) {
    return make_node(AST_FUNCTION_TYPE, token);
}

u32 AstBuilder::make_this(u32 token) {
    return make_node(AST_THIS, token);
}

u32 AstBuilder::make_list(u32 token) {
    return make_node(AST_LIST, token);
}

u32 AstBuilder::make_array(u32 token) {
    return make_node(AST_ARRAY, token);
}

u32 AstBuilder::make_hash(u32 token) {
    return make_node(AST_HASH, token);
}

u32 AstBuilder::make_hash_pair(u32 token, u32 key, u32 value) {
    u32 node = make_node(AST_HASH_PAIR, token);

    u32 last = add_child(node, 0, key);

    add_child(node, last, value);

    return node;
}

u32 AstBuilder::make_tuple(u32 token) {
    return make_node(AST_TUPLE, token);
}

u32 AstBuilder::make_closure(u32 token) {
    return make_node(AST_CLOSURE, token);
}

// unlike a 'def' parameter the type is optional, so a 0 is a real answer here
u32 AstBuilder::make_closure_parameter(u32 name, u32 type) {
    u32 node = make_node(AST_CLOSURE_PARAMETER, 0);

    u32 last = add_child(node, 0, name);

    add_child(node, last, type);

    return node;
}

// its own kind rather than AST_FUNCTION_RETURN_TYPE, because a closure writes
// it as '-> T' and a 'def' writes it as ': T'
u32 AstBuilder::make_closure_return_type(u32 child) {
    u32 node = make_node(AST_CLOSURE_RETURN_TYPE, 0);

    add_child(node, 0, child);

    return node;
}

// the token is the opening quote, and it is the only record of which quote the
// source used: the printer writes it at both ends
u32 AstBuilder::make_template_string(u32 token) {
    return make_node(AST_TEMPLATE_STRING, token);
}

u32 AstBuilder::make_template_string_chunk(u32 token) {
    return make_node(AST_TEMPLATE_STRING_CHUNK, token);
}

u32 AstBuilder::make_interpolation(u32 token, u32 expression) {
    u32 node = make_node(AST_INTERPOLATION, token);

    add_child(node, 0, expression);

    return node;
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
