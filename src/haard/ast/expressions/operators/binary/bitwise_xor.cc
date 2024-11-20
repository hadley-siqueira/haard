#include "haard/ast/expressions/operators/binary/bitwise_xor.h"

using namespace haard;

BitwiseXor::BitwiseXor() {
    set_kind(AST_BITWISE_XOR);
}

BitwiseXor::BitwiseXor(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_XOR);
    set_token(token);
    set_left(left);
    set_right(right);
}