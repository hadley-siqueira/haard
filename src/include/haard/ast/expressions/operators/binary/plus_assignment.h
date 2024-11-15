#ifndef HAARD_AST_PLUS_ASSIGNMENT_H
#define HAARD_AST_PLUS_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class PlusAssignment : public BinaryOperator {
    public:
        PlusAssignment();
        PlusAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif