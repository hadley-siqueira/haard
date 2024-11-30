#include "haard/ast/expressions/null.h"

using namespace haard;

Null::Null() {
    set_kind(AST_NULL);
}

Null::Null(Token& token) {
    set_kind(AST_NULL);
    set_token(token);
}

const Token& Null::get_token() const {
    return token;
}

void Null::set_token(const Token& token) {
    this->token = token;
}
