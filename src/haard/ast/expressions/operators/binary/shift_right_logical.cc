#include "haard/ast/expressions/operators/binary/shift_right_logical.h"

using namespace haard;

ShiftRightLogical::ShiftRightLogical() {
    set_kind(AST_SHIFT_RIGHT_LOGICAL);
}

ShiftRightLogical::ShiftRightLogical(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_RIGHT_LOGICAL);
    set_token(token);
    set_left(left);
    set_left(right);
}