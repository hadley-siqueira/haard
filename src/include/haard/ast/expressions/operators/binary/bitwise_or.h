#ifndef HAARD_AST_BITWISE_OR_H
#define HAARD_AST_BITWISE_OR_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseOr : public BinaryOperator {
    public:
        BitwiseOr();
        BitwiseOr(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif