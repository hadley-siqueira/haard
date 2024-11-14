#ifndef HAARD_AST_LESS_THAN_H
#define HAARD_AST_LESS_THAN_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class LessThan : public BinaryOperator {
    public:
        LessThan();
        LessThan(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif