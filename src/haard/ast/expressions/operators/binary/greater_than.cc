#include "haard/ast/expressions/operators/binary/greater_than.h"

using namespace haard;

GreaterThan::GreaterThan() {
    set_kind(AST_GREATER_THAN);
}

GreaterThan::GreaterThan(Token& token, Expression* left, Expression* right) {
    set_kind(AST_GREATER_THAN);
    set_token(token);
    set_left(left);
    set_left(right);
}