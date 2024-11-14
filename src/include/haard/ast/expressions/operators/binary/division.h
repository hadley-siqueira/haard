#ifndef HAARD_AST_DIVISION_H
#define HAARD_AST_DIVISION_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Division : public BinaryOperator {
    public:
        Division();
        Division(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif