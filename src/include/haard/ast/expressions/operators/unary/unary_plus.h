#ifndef HAARD_AST_UNARY_PLUS_H
#define HAARD_AST_UNARY_PLUS_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class UnaryPlus : public UnaryOperator {
    public:
        UnaryPlus();
        UnaryPlus(Token& token, Expression* expression=nullptr);
    };
}

#endif