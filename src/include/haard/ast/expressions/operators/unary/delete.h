#ifndef HAARD_AST_DELETE_H
#define HAARD_AST_DELETE_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class Delete : public UnaryOperator {
    public:
        Delete();
        Delete(Token& token, Expression* expression=nullptr);
    };
}

#endif