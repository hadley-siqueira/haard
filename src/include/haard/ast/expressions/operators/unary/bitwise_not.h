#ifndef HAARD_AST_BITWISE_NOT_H
#define HAARD_AST_BITWISE_NOT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class BitwiseNot : public UnaryOperator {
    public:
        BitwiseNot();
        BitwiseNot(Token& token, Expression* expression=nullptr);
    };
}

#endif