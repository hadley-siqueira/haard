#include "ast/this.h"

using namespace haard;

This::This() {
    set_kind(AST_THIS);
}

This::This(Token& token) {
    set_kind(AST_THIS);
    set_token(token);
}
