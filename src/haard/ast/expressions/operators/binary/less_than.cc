#include "haard/ast/expressions/operators/binary/less_than.h"

using namespace haard;

LessThan::LessThan() {
    set_kind(AST_LESS_THAN);
}

LessThan::LessThan(Token& token, Expression* left, Expression* right) {
    set_kind(AST_LESS_THAN);
    set_token(token);
    set_left(left);
    set_right(right);
}