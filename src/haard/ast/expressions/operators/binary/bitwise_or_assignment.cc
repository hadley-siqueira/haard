#include "haard/ast/expressions/operators/binary/bitwise_or_assignment.h"

using namespace haard;

BitwiseOrAssignment::BitwiseOrAssignment() {
    set_kind(AST_BITWISE_OR_ASSIGNMENT);
}

BitwiseOrAssignment::BitwiseOrAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_OR_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}