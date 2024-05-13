#ifndef HAARD_AST_DOUBLE_DOLAR_H
#define HAARD_AST_DOUBLE_DOLAR_H

#include "unary_operator.h"

namespace haard {
    class DoubleDolar : public UnaryOperator {
    public:
        DoubleDolar(Expression* expression=nullptr);
        DoubleDolar(Token& token, Expression* expression=nullptr);
    };
}

#endif
