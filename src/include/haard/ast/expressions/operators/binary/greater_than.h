#ifndef HAARD_AST_GREATER_THAN_H
#define HAARD_AST_GREATER_THAN_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class GreaterThan : public BinaryOperator {
    public:
        GreaterThan();
        GreaterThan(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif