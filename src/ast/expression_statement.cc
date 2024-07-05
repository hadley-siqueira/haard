#include "ast/expression_statement.h"

using namespace haard;

ExpressionStatement::ExpressionStatement(Expression* expr) {
    set_kind(AST_EXPRESSION);
    set_expression(expr);
}

ExpressionStatement::~ExpressionStatement() {
    delete expression;
}

void ExpressionStatement::set_expression(Expression* expr) {
    this->expression = expr;
}

Expression* ExpressionStatement::get_expression() {
    return expression;
}
