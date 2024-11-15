#ifndef HAARD_AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT_H
#define HAARD_AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftRightLogicalAssignment : public BinaryOperator {
    public:
        ShiftRightLogicalAssignment();
        ShiftRightLogicalAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif