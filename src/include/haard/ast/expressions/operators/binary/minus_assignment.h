#ifndef HAARD_AST_MINUS_ASSIGNMENT_H
#define HAARD_AST_MINUS_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class MinusAssignment : public BinaryOperator {
    public:
        MinusAssignment();
        MinusAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif