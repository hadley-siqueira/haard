#ifndef HAARD_AST_PLUS_H
#define HAARD_AST_PLUS_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Plus : public BinaryOperator {
    public:
        Plus();
        Plus(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif