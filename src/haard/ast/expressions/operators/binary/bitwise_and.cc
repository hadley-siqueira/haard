#include "haard/ast/expressions/operators/binary/bitwise_and.h"

using namespace haard;

BitwiseAnd::BitwiseAnd() {
    set_kind(AST_BITWISE_AND);
}

BitwiseAnd::BitwiseAnd(Token& token, Expression* left, Expression* right) {
    set_kind(AST_BITWISE_AND);
    set_token(token);
    set_left(left);
    set_right(right);
}