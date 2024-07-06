#include <sstream>
#include "ast/binary_operators/hash_pair.h"

using namespace haard;

HashPair::HashPair(Expression* left, Expression* right) {
    set_kind(AST_HASH_PAIR);
    set_left(left);
    set_right(right);
}

HashPair::HashPair(Token& token, Expression* left, Expression* right) {
    set_kind(AST_HASH_PAIR);
    set_left(left);
    set_right(right);
    set_token(token);
}
