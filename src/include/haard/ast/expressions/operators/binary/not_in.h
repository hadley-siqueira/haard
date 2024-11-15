#ifndef HAARD_AST_NOT_IN_H
#define HAARD_AST_NOT_IN_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class NotIn : public BinaryOperator {
    public:
        NotIn();
        NotIn(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif