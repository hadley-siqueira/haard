#ifndef HAARD_AST_SHIFT_RIGHT_LOGICAL_H
#define HAARD_AST_SHIFT_RIGHT_LOGICAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftRightLogical : public BinaryOperator {
    public:
        ShiftRightLogical();
        ShiftRightLogical(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif