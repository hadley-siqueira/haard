#include <haard/ast/expression_statement.h>

using namespace haard;

ExpressionStatement::ExpressionStatement(Expression* expression) {
    set_kind(AST_UNKNOWN);
    set_expression(expression);
    set_semicolon(false);
}

ExpressionStatement::~ExpressionStatement() {
    delete expression;
}

void ExpressionStatement::set_expression(Expression* expression) {
    if (expression) {
        expression->set_parent_node(this);
        this->expression = expression;
    }
}

Expression* ExpressionStatement::get_expression() {
    return expression;
}

void ExpressionStatement::set_semicolon(bool value) {
    semicolon_flag = value;
}

bool ExpressionStatement::has_semicolon() {
    return semicolon_flag;
}
