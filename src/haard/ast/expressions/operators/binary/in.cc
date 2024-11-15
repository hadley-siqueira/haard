#include "haard/ast/expressions/operators/binary/in.h"

using namespace haard;

In::In() {
    set_kind(AST_IN);
}

In::In(Token& token, Expression* left, Expression* right) {
    set_kind(AST_IN);
    set_token(token);
    set_left(left);
    set_left(right);
}