#include "haard/ast/expressions/operators/binary/bitwise_xor_assignment.h"

using namespace haard;

BitwiseXorAssignment::BitwiseXorAssignment() {
    set_kind(AST_BITWISE_XOR_ASSIGNMENT);
}

BitwiseXorAssignment::BitwiseXorAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_XOR_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_right(right);
}