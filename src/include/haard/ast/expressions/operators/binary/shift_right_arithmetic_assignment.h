#ifndef HAARD_AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT_H
#define HAARD_AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftRightArithmeticAssignment : public BinaryOperator {
    public:
        ShiftRightArithmeticAssignment();
        ShiftRightArithmeticAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif