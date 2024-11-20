#ifndef HAARD_AST_DEREFERENCE_H
#define HAARD_AST_DEREFERENCE_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class Dereference : public UnaryOperator {
    public:
        Dereference();
        Dereference(Token& token, Expression* expression=nullptr);
    };
}

#endif