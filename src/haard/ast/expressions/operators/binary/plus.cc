#include "haard/ast/expressions/operators/binary/plus.h"

using namespace haard;

Plus::Plus() {
    set_kind(AST_PLUS);
}

Plus::Plus(Token& token, Expression* left, Expression* right) {
    set_kind(AST_PLUS);
    set_token(token);
    set_left(left);
    set_right(right);
}