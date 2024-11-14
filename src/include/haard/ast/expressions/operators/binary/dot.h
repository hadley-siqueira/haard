#ifndef HAARD_AST_DOT_H
#define HAARD_AST_DOT_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Dot : public BinaryOperator {
    public:
        Dot();
        Dot(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif
