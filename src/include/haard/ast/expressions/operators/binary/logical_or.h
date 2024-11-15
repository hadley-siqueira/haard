#ifndef HAARD_AST_LOGICAL_OR_H
#define HAARD_AST_LOGICAL_OR_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class LogicalOr : public BinaryOperator {
    public:
        LogicalOr();
        LogicalOr(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif