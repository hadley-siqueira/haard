#ifndef HAARD_AST_BITWISE_OR_ASSIGNMENT_H
#define HAARD_AST_BITWISE_OR_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseOrAssignment : public BinaryOperator {
    public:
        BitwiseOrAssignment();
        BitwiseOrAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif