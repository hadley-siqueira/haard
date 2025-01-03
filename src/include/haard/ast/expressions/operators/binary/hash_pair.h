#ifndef HAARD_AST_HASH_PAIR_H
#define HAARD_AST_HASH_PAIR_H

#include "haard/ast/expressions/operators/binary/binary_operator.h"

namespace haard {
    class HashPair : public BinaryOperator {
    public:
        HashPair();
        HashPair(Token& token, Expression* left=nullptr, Expression* right=nullptr);
    };
}

#endif
