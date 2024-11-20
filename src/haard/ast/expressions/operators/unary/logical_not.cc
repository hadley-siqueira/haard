#include "haard/ast/expressions/operators/unary/logical_not.h"

using namespace haard;

LogicalNot::LogicalNot() {
    set_kind(AST_LOGICAL_NOT);
}

LogicalNot::LogicalNot(Token& token, Expression* expression) {
    set_kind(AST_LOGICAL_NOT);
    set_token(token);
    set_expression(expression);
}