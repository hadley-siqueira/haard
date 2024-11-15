#ifndef HAARD_AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT_H
#define HAARD_AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftLeftLogicalAssignment : public BinaryOperator {
    public:
        ShiftLeftLogicalAssignment();
        ShiftLeftLogicalAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif