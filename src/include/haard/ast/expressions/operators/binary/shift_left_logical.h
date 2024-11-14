#ifndef HAARD_AST_SHIFT_LEFT_LOGICAL_H
#define HAARD_AST_SHIFT_LEFT_LOGICAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ShiftLeftLogical : public BinaryOperator {
    public:
        ShiftLeftLogical();
        ShiftLeftLogical(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif