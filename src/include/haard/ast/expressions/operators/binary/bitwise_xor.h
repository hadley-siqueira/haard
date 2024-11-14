#ifndef HAARD_AST_BITWISE_XOR_H
#define HAARD_AST_BITWISE_XOR_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseXor : public BinaryOperator {
    public:
        BitwiseXor();
        BitwiseXor(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif