#ifndef HAARD_AST_NOT_EQUAL_H
#define HAARD_AST_NOT_EQUAL_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class NotEqual : public BinaryOperator {
    public:
        NotEqual();
        NotEqual(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif