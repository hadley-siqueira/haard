#ifndef HAARD_AST_PRE_INCREMENT_H
#define HAARD_AST_PRE_INCREMENT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class PreIncrement : public UnaryOperator {
    public:
        PreIncrement();
        PreIncrement(Token& token, Expression* expression=nullptr);
    };
}

#endif