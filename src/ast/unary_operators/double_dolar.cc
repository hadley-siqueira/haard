#include "ast/unary_operators/double_dolar.h"

using namespace haard;

DoubleDolar::DoubleDolar(Expression* expression) {
    set_kind(EXPR_DOUBLE_DOLAR);
    set_expression(expression);
}

DoubleDolar::DoubleDolar(Token& token, Expression* expression) {
    set_kind(EXPR_DOUBLE_DOLAR);
    set_expression(expression);
    set_token(token);
}
