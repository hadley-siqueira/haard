#ifndef HAARD_AST_GREATER_THAN_OR_EQUAL_H
#define HAARD_AST_GREATER_THAN_OR_EQUAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class GreaterThanOrEqual : public BinaryOperator {
    public:
        GreaterThanOrEqual();
        GreaterThanOrEqual(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif