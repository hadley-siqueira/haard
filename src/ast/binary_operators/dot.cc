#include "ast/binary_operators/dot.h"

using namespace haard;

Dot::Dot(Expression* left, Expression* right) {
    set_kind(AST_DOT);
    set_left(left);
    set_right(right);
}

Dot::Dot(Token& token, Expression* left, Expression* right) {
    set_kind(AST_DOT);
    set_left(left);
    set_right(right);
    set_token(token);
}
