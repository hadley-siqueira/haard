#include "haard/ast/expressions/operators/binary/bitwise_and_assignment.h"

using namespace haard;

BitwiseAndAssignment::BitwiseAndAssignment() {
    set_kind(AST_BITWISE_AND_ASSIGNMENT);
}

BitwiseAndAssignment::BitwiseAndAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_AND_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}