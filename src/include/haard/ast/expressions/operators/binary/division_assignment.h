#ifndef HAARD_AST_DIVISION_ASSIGNMENT_H
#define HAARD_AST_DIVISION_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class DivisionAssignment : public BinaryOperator {
    public:
        DivisionAssignment();
        DivisionAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif