#include "haard/ast/expressions/operators/binary/exclusive_range.h"

using namespace haard;

ExclusiveRange::ExclusiveRange() {
    set_kind(AST_EXCLUSIVE_RANGE);
}

ExclusiveRange::ExclusiveRange(Token& token, Expression* left, Expression* right) {
    set_kind(AST_EXCLUSIVE_RANGE);
    set_token(token);
    set_left(left);
    set_right(right);
}