#include "haard/ast/statements/expression_statement.h"

using namespace haard;

ExpressionStatement::ExpressionStatement() {
    set_kind(AST_STATEMENT_EXPRESSION);
    set_expression(nullptr);
}

ExpressionStatement::ExpressionStatement(Expression* expression) {
    set_kind(AST_STATEMENT_EXPRESSION);
    set_expression(expression);
}

ExpressionStatement::~ExpressionStatement() {
    delete expression;
}

Expression* ExpressionStatement::get_expression() const {
    return expression;
}

void ExpressionStatement::set_expression(Expression* expression) {
    this->expression = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

std::string ExpressionStatement::to_json() {
    return "expression statement json";
}

std::string ExpressionStatement::to_str() {
    return expression->to_str();
}
