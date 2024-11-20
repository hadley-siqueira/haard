#ifndef HAARD_AST_UNARY_MINUS_H
#define HAARD_AST_UNARY_MINUS_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class UnaryMinus : public UnaryOperator {
    public:
        UnaryMinus();
        UnaryMinus(Token& token, Expression* expression=nullptr);
    };
}

#endif