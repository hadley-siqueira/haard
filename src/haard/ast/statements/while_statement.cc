#include <sstream>

#include "haard/ast/statements/while_statement.h"

using namespace haard;

WhileStatement::WhileStatement() {
    set_kind(AST_WHILE);
    set_expression(nullptr);
    set_statements(nullptr);
}

WhileStatement::~WhileStatement() {
    delete statements;
    delete expression;
}

Statements* WhileStatement::get_statements() const {
    return statements;
}

void WhileStatement::set_statements(Statements* statements) {
    this->statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

Expression* WhileStatement::get_expression() const {
    return expression;
}

void WhileStatement::set_expression(Expression* expression) {
    this->expression = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

const Token& WhileStatement::get_token() const {
    return token;
}

void WhileStatement::set_token(const Token& token) {
    this->token = token;
}
