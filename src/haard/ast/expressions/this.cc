#include "haard/ast/expressions/this.h"

using namespace haard;

This::This() {
    set_kind(AST_THIS);
}

This::This(Token& token) {
    set_kind(AST_THIS);
    set_token(token);
}

const Token& This::get_token() const {
    return token;
}

void This::set_token(const Token& token) {
    this->token = token;
}
