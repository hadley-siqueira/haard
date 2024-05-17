#include "ast/this.h"

using namespace haard;

This::This() {
    set_kind(EXPR_THIS);
}

This::This(Token& token) {
    set_kind(EXPR_THIS);
    set_token(token);
}
