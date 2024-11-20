#include "haard/ast/expressions/operators/binary/shift_left_logical.h"

using namespace haard;

ShiftLeftLogical::ShiftLeftLogical() {
    set_kind(AST_SHIFT_LEFT_LOGICAL);
}

ShiftLeftLogical::ShiftLeftLogical(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_LEFT_LOGICAL);
    set_token(token);
    set_left(left);
    set_right(right);
}