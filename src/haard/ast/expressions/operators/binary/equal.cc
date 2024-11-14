#include "haard/ast/expressions/operators/binary/equal.h"

using namespace haard;

Equal::Equal() {
    set_kind(AST_EQUAL);
}

Equal::Equal(Token& token, Expression* left, Expression* right) {
    set_kind(AST_EQUAL);
    set_token(token);
    set_left(left);
    set_left(right);
}