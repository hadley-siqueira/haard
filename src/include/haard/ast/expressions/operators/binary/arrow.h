#ifndef HAARD_AST_ARROW_H
#define HAARD_AST_ARROW_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Arrow : public BinaryOperator {
    public:
        Arrow();
        Arrow(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif