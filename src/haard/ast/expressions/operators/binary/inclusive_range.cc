#include "haard/ast/expressions/operators/binary/inclusive_range.h"

using namespace haard;

InclusiveRange::InclusiveRange() {
    set_kind(AST_INCLUSIVE_RANGE);
}

InclusiveRange::InclusiveRange(Token& token, Expression* left, Expression* right) {
    set_kind(AST_INCLUSIVE_RANGE);
    set_token(token);
    set_left(left);
    set_right(right);
}