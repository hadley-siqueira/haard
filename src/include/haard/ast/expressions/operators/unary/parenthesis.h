#ifndef HAARD_AST_PARENTHESIS_H
#define HAARD_AST_PARENTHESIS_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class Parenthesis : public UnaryOperator {
    public:
        Parenthesis();
        Parenthesis(Token& token, Expression* expression=nullptr);
    };
}

#endif