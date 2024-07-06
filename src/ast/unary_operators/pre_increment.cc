#include "ast/unary_operators/pre_increment.h"

using namespace haard;

PreIncrement::PreIncrement(Expression* expression) {
    set_kind(AST_PRE_INCREMENT);
    set_expression(expression);
}

PreIncrement::PreIncrement(Token& token, Expression* expression) {
    set_kind(AST_PRE_INCREMENT);
    set_expression(expression);
    set_token(token);
}
