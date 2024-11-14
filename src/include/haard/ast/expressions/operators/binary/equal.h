#ifndef HAARD_AST_EQUAL_H
#define HAARD_AST_EQUAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Equal : public BinaryOperator {
    public:
        Equal();
        Equal(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif