#include "haard/ast/expressions/literals/literal.h"

using namespace haard;

const Token& Literal::get_token() const {
    return token;
}

void Literal::set_token(const Token& token) {
    this->token = token;
}
