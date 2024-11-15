#ifndef HAARD_AST_BITWISE_AND_ASSIGNMENT_H
#define HAARD_AST_BITWISE_AND_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseAndAssignment : public BinaryOperator {
    public:
        BitwiseAndAssignment();
        BitwiseAndAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif