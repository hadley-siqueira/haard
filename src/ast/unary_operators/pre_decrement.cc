#include "ast/unary_operators/pre_decrement.h"

using namespace haard;

PreDecrement::PreDecrement(Expression* expression) {
    set_kind(AST_PRE_DECREMENT);
    set_expression(expression);
}

PreDecrement::PreDecrement(Token& token, Expression* expression) {
    set_kind(AST_PRE_DECREMENT);
    set_expression(expression);
    set_token(token);
}
