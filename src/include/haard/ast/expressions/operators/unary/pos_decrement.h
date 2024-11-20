#ifndef HAARD_AST_POS_DECREMENT_H
#define HAARD_AST_POS_DECREMENT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class PosDecrement : public UnaryOperator {
    public:
        PosDecrement();
        PosDecrement(Token& token, Expression* expression=nullptr);
    };
}

#endif