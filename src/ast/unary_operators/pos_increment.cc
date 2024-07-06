#include "ast/unary_operators/pos_increment.h"

using namespace haard;

PosIncrement::PosIncrement(Expression* expression) {
    set_kind(AST_POS_INCREMENT);
    set_expression(expression);
}

PosIncrement::PosIncrement(Token& token, Expression* expression) {
    set_kind(AST_POS_INCREMENT);
    set_expression(expression);
    set_token(token);
}
