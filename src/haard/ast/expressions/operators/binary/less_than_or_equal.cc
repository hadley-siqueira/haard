#include "haard/ast/expressions/operators/binary/less_than_or_equal.h"

using namespace haard;

LessThanOrEqual::LessThanOrEqual() {
    set_kind(AST_LESS_THAN_OR_EQUAL);
}

LessThanOrEqual::LessThanOrEqual(Token& token, Expression* left, Expression* right) {
    set_kind(AST_LESS_THAN_OR_EQUAL);
    set_token(token);
    set_left(left);
    set_right(right);
}