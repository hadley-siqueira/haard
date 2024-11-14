#ifndef HAARD_AST_SHIFT_RIGHT_ARITHMETIC_H
#define HAARD_AST_SHIFT_RIGHT_ARITHMETIC_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftRightArithmetic : public BinaryOperator {
    public:
        ShiftRightArithmetic();
        ShiftRightArithmetic(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif