#ifndef HAARD_AST_ASSIGNMENT_H
#define HAARD_AST_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Assignment : public BinaryOperator {
    public:
        Assignment();
        Assignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif
