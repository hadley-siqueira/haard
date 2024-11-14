#include "haard/ast/expressions/operators/binary/minus.h"

using namespace haard;

Minus::Minus() {
    set_kind(AST_MINUS);
}

Minus::Minus(Token& token, Expression* left, Expression* right) {
    set_kind(AST_MINUS);
    set_token(token);
    set_left(left);
    set_left(right);
}