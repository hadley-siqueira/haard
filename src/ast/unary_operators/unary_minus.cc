#include "ast/unary_operators/unary_minus.h"

using namespace haard;

UnaryMinus::UnaryMinus(Expression* expression) {
    set_kind(AST_UNARY_MINUS);
    set_expression(expression);
}

UnaryMinus::UnaryMinus(Token& token, Expression* expression) {
    set_kind(AST_UNARY_MINUS);
    set_expression(expression);
    set_token(token);
}
