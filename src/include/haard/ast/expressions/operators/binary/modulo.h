#ifndef HAARD_AST_MODULO_H
#define HAARD_AST_MODULO_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Modulo : public BinaryOperator {
    public:
        Modulo();
        Modulo(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif