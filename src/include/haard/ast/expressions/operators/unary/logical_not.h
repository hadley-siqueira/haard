#ifndef HAARD_AST_LOGICAL_NOT_H
#define HAARD_AST_LOGICAL_NOT_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class LogicalNot : public UnaryOperator {
    public:
        LogicalNot();
        LogicalNot(Token& token, Expression* expression=nullptr);
    };
}

#endif