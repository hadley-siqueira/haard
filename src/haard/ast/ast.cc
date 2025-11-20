#include <haard/ast/ast.h>

using namespace haard;

Ast::Ast() {
    set_kind(AST_UNKNOWN);
}

Ast::Ast(AstKind kind) {
    set_kind(kind);
}

Ast::~Ast() {

}

void Ast::set_kind(AstKind kind) {
    this->kind = kind;
}

void Ast::set_token(Token& token) {
    this->token = token;
}

AstKind Ast::get_kind() {
    return kind;
}

Token Ast::get_token() {
    return token;
}
