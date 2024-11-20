#include "haard/ast/expressions/operators/binary/shift_right_logical_assignment.h"

using namespace haard;

ShiftRightLogicalAssignment::ShiftRightLogicalAssignment() {
    set_kind(AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT);
}

ShiftRightLogicalAssignment::ShiftRightLogicalAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}