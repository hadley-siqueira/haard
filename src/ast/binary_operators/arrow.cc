#include "ast/binary_operators/arrow.h"

using namespace haard;

Arrow::Arrow(Expression* left, Expression* right) {
    set_kind(AST_ARROW);
    set_left(left);
    set_right(right);
}

Arrow::Arrow(Token& token, Expression* left, Expression* right) {
    set_kind(AST_ARROW);
    set_left(left);
    set_right(right);
    set_token(token);
}
