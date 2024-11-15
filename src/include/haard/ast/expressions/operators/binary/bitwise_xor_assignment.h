#ifndef HAARD_AST_BITWISE_XOR_ASSIGNMENT_H
#define HAARD_AST_BITWISE_XOR_ASSIGNMENT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class BitwiseXorAssignment : public BinaryOperator {
    public:
        BitwiseXorAssignment();
        BitwiseXorAssignment(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif