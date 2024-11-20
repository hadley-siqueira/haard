#include "haard/ast/expressions/operators/binary/bitwise_not_assignment.h"

using namespace haard;

BitwiseNotAssignment::BitwiseNotAssignment() {
    set_kind(AST_BITWISE_NOT_ASSIGNMENT);
}

BitwiseNotAssignment::BitwiseNotAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_NOT_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}