#include <haard/ast/literal.h>

using namespace haard;

Literal::Literal() {
    set_kind(AST_UNKNOWN);
}

Literal::Literal(const Token& token) {
    set_kind(AST_LITERAL);
    set_token(token);
}

Literal::~Literal() {

}

void Literal::set_token(const Token& token) {
    this->token = token;
}

Token& Literal::get_token() {
    return token;
}
