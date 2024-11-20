#include "haard/ast/expressions/operators/binary/greater_than_or_equal.h"

using namespace haard;

GreaterThanOrEqual::GreaterThanOrEqual() {
    set_kind(AST_GREATER_THAN_OR_EQUAL);
}

GreaterThanOrEqual::GreaterThanOrEqual(Token& token, Expression* left, Expression* right) {
    set_kind(AST_GREATER_THAN_OR_EQUAL);
    set_token(token);
    set_left(left);
    set_right(right);
}