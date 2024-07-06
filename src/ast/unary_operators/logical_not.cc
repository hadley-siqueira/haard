#include "ast/unary_operators/logical_not.h"

using namespace haard;

LogicalNot::LogicalNot(Expression* expression) {
    set_kind(AST_LOGICAL_NOT);
    set_expression(expression);
}

LogicalNot::LogicalNot(Token& token, Expression* expression) {
    set_kind(AST_LOGICAL_NOT);
    set_expression(expression);
    set_token(token);
}
