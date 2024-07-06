#include "ast/unary_operators/dereference.h"

using namespace haard;

Dereference::Dereference(Expression* expression) {
    set_kind(AST_DEREFERENCE);
    set_expression(expression);
}

Dereference::Dereference(Token& token, Expression* expression) {
    set_kind(AST_DEREFERENCE);
    set_expression(expression);
    set_token(token);
}
