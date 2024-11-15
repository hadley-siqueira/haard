#include "haard/ast/expressions/operators/binary/shift_right_arithmetic_assignment.h"

using namespace haard;

ShiftRightArithmeticAssignment::ShiftRightArithmeticAssignment() {
    set_kind(AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT);
}

ShiftRightArithmeticAssignment::ShiftRightArithmeticAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}