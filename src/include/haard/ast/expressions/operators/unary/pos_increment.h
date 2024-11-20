#ifndef HAARD_AST_POS_INCREMENT_H
#define HAARD_AST_POS_INCREMENT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class PosIncrement : public UnaryOperator {
    public:
        PosIncrement();
        PosIncrement(Token& token, Expression* expression=nullptr);
    };
}

#endif