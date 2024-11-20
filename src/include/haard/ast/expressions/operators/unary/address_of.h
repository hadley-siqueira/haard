#ifndef HAARD_AST_ADDRESS_OF_H
#define HAARD_AST_ADDRESS_OF_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class AddressOf : public UnaryOperator {
    public:
        AddressOf();
        AddressOf(Token& token, Expression* expression=nullptr);
    };
}

#endif