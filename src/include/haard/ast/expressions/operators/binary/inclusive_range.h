#ifndef HAARD_AST_INCLUSIVE_RANGE_H
#define HAARD_AST_INCLUSIVE_RANGE_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class InclusiveRange : public BinaryOperator {
    public:
        InclusiveRange();
        InclusiveRange(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif