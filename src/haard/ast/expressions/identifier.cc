#include "haard/ast/expressions/identifier.h"

using namespace haard;

Identifier::Identifier() {
    set_kind(AST_IDENTIFIER);
}

Identifier::Identifier(Token& token) {
    set_kind(AST_IDENTIFIER);
    set_token(token);
}

Identifier::~Identifier() {

}

const Token& Identifier::get_token() const {
    return token;
}

void Identifier::set_token(const Token& token) {
    this->token = token;
}
