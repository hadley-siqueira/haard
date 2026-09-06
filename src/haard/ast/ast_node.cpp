#include <haard/ast/ast_node.h>

using namespace haard;

AstNode::AstNode() {
    set_kind(AST_UNKNOWN);
    set_token(0);
    set_sibling(0);
    set_children(0);
}

void AstNode::set_kind(AstNodeKind kind) {
    this->kind = (u8) kind;
}

AstNodeKind AstNode::get_kind() {
    return (AstNodeKind) kind;
}

void AstNode::set_token(u32 token) {
    this->token = token;
}

u32 AstNode::get_token() {
    return token;
}

void AstNode::set_sibling(u32 sibling) {
    this->sibling = sibling;
}

u32 AstNode::get_sibling() {
    return sibling;
}

void AstNode::set_children(u32 child) {
    this->children = child;
}

u32 AstNode::get_children() {
    return children;
}


const char* haard::operator_name(AstNodeKind kind) {
    switch (kind) {
    case AST_ASSIGNMENT: return "operator=";
    case AST_INDEX: return "operator[]";
    case AST_PLUS: return "operator+";
    case AST_MINUS: return "operator-";
    case AST_TIMES: return "operator*";
    case AST_DIVISION: return "operator/";
    case AST_INTEGER_DIVISION: return "operator//";
    case AST_MODULO: return "operator%";
    case AST_EQUAL: return "operator==";
    case AST_NOT_EQUAL: return "operator!=";
    case AST_LESS_THAN: return "operator<";
    case AST_LESS_THAN_OR_EQUAL: return "operator<=";
    case AST_GREATER_THAN: return "operator>";
    case AST_GREATER_THAN_OR_EQUAL: return "operator>=";

    default:
        break;
    }

    return nullptr;
}
