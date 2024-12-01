#ifndef HAARD_AST_DELETE_ARRAY_H
#define HAARD_AST_DELETE_ARRAY_H

#include "haard/ast/expressions/operators/unary/unary_operator.h"

namespace haard {
    class DeleteArray : public UnaryOperator {
    public:
        DeleteArray();
        DeleteArray(Token& token, Expression* expression=nullptr);
    };
}

#endif