#ifndef HAARD_AST_IN_H
#define HAARD_AST_IN_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class In : public BinaryOperator {
    public:
        In();
        In(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif