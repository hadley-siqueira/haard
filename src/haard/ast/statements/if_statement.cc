#include <sstream>

#include "haard/ast/statements/if_statement.h"

using namespace haard;

IfStatement::IfStatement() {
    set_kind(AST_IF);
    set_expression(nullptr);
    set_true_statements(nullptr);
    set_false_statements(nullptr);
}

IfStatement::~IfStatement() {
    delete true_statements;
    delete false_statements;
    delete expression;
}

Statement* IfStatement::get_true_statements() const {
    return true_statements;
}

void IfStatement::set_true_statements(Statement* statements) {
    this->true_statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

Statement* IfStatement::get_false_statements() const {
    return false_statements;
}

void IfStatement::set_false_statements(Statement* statements) {
    this->false_statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

Expression* IfStatement::get_expression() const {
    return expression;
}

void IfStatement::set_expression(Expression* expression) {
    this->expression = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

const Token& IfStatement::get_token() const {
    return token;
}

void IfStatement::set_token(const Token& token) {
    this->token = token;
}
