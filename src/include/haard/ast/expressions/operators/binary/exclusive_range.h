#ifndef HAARD_AST_EXCLUSIVE_RANGE_H
#define HAARD_AST_EXCLUSIVE_RANGE_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class ExclusiveRange : public BinaryOperator {
    public:
        ExclusiveRange();
        ExclusiveRange(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif