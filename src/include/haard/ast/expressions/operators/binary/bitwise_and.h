#ifndef HAARD_AST_BITWISE_AND_H
#define HAARD_AST_BITWISE_AND_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseAnd : public BinaryOperator {
    public:
        BitwiseAnd();
        BitwiseAnd(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif