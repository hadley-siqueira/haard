#include "haard/ast/expressions/operators/binary/hash_pair.h"

using namespace haard;

HashPair::HashPair() {
    set_kind(AST_HASH_PAIR);
}

HashPair::HashPair(Token& token, Expression* left, Expression* right) {
    set_kind(AST_HASH_PAIR);
    set_token(token);
    set_left(left);
    set_right(right);
}
