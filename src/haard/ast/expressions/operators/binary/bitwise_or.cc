#include "haard/ast/expressions/operators/binary/bitwise_or.h"

using namespace haard;

BitwiseOr::BitwiseOr() {
    set_kind(AST_BITWISE_OR);
}

BitwiseOr::BitwiseOr(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_OR);
    set_token(token);
    set_left(left);
    set_left(right);
}