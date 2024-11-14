#ifndef HAARD_AST_TIMES_H
#define HAARD_AST_TIMES_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Times : public BinaryOperator {
    public:
        Times();
        Times(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif