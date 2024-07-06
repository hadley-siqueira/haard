#include "ast/unary_operators/bitwise_not.h"

using namespace haard;

BitwiseNot::BitwiseNot(Expression* expression) {
    set_kind(AST_BITWISE_NOT);
    set_expression(expression);
}

BitwiseNot::BitwiseNot(Token& token, Expression* expression) {
    set_kind(AST_BITWISE_NOT);
    set_expression(expression);
    set_token(token);
}
