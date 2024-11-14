#include "haard/ast/expressions/operators/binary/arrow.h"

using namespace haard;

Arrow::Arrow() {
    set_kind(AST_ARROW);
}

Arrow::Arrow(Token& token, Expression* left, Expression* right) {
    set_kind(AST_ARROW);
    set_token(token);
    set_left(left);
    set_left(right);
}