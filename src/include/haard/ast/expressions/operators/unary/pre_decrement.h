#ifndef HAARD_AST_PRE_DECREMENT_H
#define HAARD_AST_PRE_DECREMENT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class PreDecrement : public UnaryOperator {
    public:
        PreDecrement();
        PreDecrement(Token& token, Expression* expression=nullptr);
    };
}

#endif