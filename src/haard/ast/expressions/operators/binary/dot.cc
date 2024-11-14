#include "haard/ast/expressions/operators/binary/dot.h"

using namespace haard;

Dot::Dot() {
    set_kind(AST_DOT);
}

Dot::Dot(Token& token, Expression* left, Expression* right) {
    set_kind(AST_DOT);
    set_token(token);
    set_left(left);
    set_left(right);
}