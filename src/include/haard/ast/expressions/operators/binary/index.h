#ifndef HAARD_AST_INDEX_H
#define HAARD_AST_INDEX_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class Index : public BinaryOperator {
    public:
        Index();
        Index(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif