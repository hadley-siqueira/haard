#ifndef HAARD_AST_POWER_H
#define HAARD_AST_POWER_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Power : public BinaryOperator {
    public:
        Power();
        Power(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif