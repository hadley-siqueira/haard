#ifndef HAARD_AST_LESS_THAN_OR_EQUAL_H
#define HAARD_AST_LESS_THAN_OR_EQUAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class LessThanOrEqual : public BinaryOperator {
    public:
        LessThanOrEqual();
        LessThanOrEqual(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif