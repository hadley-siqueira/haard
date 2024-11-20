#include "haard/ast/expressions/operators/binary/logical_or.h"

using namespace haard;

LogicalOr::LogicalOr() {
    set_kind(AST_LOGICAL_OR);
}

LogicalOr::LogicalOr(Token& token, Expression* left, Expression* right) {
    set_kind(AST_LOGICAL_OR);
    set_token(token);
    set_left(left);
    set_right(right);
}