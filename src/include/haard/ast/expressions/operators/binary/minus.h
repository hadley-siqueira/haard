#ifndef HAARD_AST_MINUS_H
#define HAARD_AST_MINUS_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Minus : public BinaryOperator {
    public:
        Minus();
        Minus(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif