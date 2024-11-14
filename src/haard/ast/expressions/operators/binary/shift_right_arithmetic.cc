#include "haard/ast/expressions/operators/binary/shift_right_arithmetic.h"

using namespace haard;

ShiftRightArithmetic::ShiftRightArithmetic() {
    set_kind(AST_SHIFT_RIGHT_ARITHMETIC);
}

ShiftRightArithmetic::ShiftRightArithmetic(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_RIGHT_ARITHMETIC);
    set_token(token);
    set_left(left);
    set_left(right);
}