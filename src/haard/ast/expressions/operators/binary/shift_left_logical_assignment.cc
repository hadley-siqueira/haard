#include "haard/ast/expressions/operators/binary/shift_left_logical_assignment.h"

using namespace haard;

ShiftLeftLogicalAssignment::ShiftLeftLogicalAssignment() {
    set_kind(AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT);
}

ShiftLeftLogicalAssignment::ShiftLeftLogicalAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}