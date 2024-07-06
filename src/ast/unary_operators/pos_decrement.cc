#include "ast/unary_operators/pos_decrement.h"

using namespace haard;

PosDecrement::PosDecrement(Expression* expression) {
    set_kind(AST_POS_DECREMENT);
    set_expression(expression);
}

PosDecrement::PosDecrement(Token& token, Expression* expression) {
    set_kind(AST_POS_DECREMENT);
    set_expression(expression);
    set_token(token);
}
