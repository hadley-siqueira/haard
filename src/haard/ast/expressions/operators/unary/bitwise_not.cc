#include "haard/ast/expressions/operators/unary/bitwise_not.h"

using namespace haard;

BitwiseNot::BitwiseNot() {
    set_kind(AST_BITWISE_NOT);
}

BitwiseNot::BitwiseNot(Token& token, Expression* expression) {
    set_kind(AST_BITWISE_NOT);
    set_token(token);
    set_expression(expression);
}