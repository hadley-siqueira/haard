#include "haard/ast/expressions/operators/binary/not_in.h"

using namespace haard;

NotIn::NotIn() {
    set_kind(AST_NOT_IN);
}

NotIn::NotIn(Token& token, Expression* left, Expression* right) {
    set_kind(AST_NOT_IN);
    set_token(token);
    set_left(left);
    set_left(right);
}