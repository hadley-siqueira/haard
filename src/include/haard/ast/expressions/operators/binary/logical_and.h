#ifndef HAARD_AST_LOGICAL_AND_H
#define HAARD_AST_LOGICAL_AND_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class LogicalAnd : public BinaryOperator {
    public:
        LogicalAnd();
        LogicalAnd(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif