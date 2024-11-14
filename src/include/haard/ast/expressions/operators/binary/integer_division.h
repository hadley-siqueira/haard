#ifndef HAARD_AST_INTEGER_DIVISION_H
#define HAARD_AST_INTEGER_DIVISION_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class IntegerDivision : public BinaryOperator {
    public:
        IntegerDivision();
        IntegerDivision(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif