#include "haard/ast/expressions/operators/binary/logical_and.h"

using namespace haard;

LogicalAnd::LogicalAnd() {
    set_kind(AST_LOGICAL_AND);
}

LogicalAnd::LogicalAnd(Token& token, Expression* left, Expression* right) {
    set_kind(AST_LOGICAL_AND);
    set_token(token);
    set_left(left);
    set_right(right);
}