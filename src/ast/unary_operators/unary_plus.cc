#include "ast/unary_operators/unary_plus.h"

using namespace haard;

UnaryPlus::UnaryPlus(Expression* expression) {
    set_kind(AST_UNARY_PLUS);
    set_expression(expression);
}

UnaryPlus::UnaryPlus(Token& token, Expression* expression) {
    set_kind(AST_UNARY_PLUS);
    set_expression(expression);
    set_token(token);
}
