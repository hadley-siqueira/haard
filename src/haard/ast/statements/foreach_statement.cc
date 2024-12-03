#include <sstream>

#include "haard/ast/statements/foreach_statement.h"

using namespace haard;

ForeachStatement::ForeachStatement() {
    set_kind(AST_FOREACH);
    set_expression(nullptr);
    set_statements(nullptr);
}

ForeachStatement::~ForeachStatement() {
    delete statements;
    delete expression;
}

Statements* ForeachStatement::get_statements() const {
    return statements;
}

void ForeachStatement::set_statements(Statements* statements) {
    this->statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

Expression* ForeachStatement::get_expression() const {
    return expression;
}

void ForeachStatement::set_expression(Expression* expression) {
    this->expression = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

const Token& ForeachStatement::get_token() const {
    return token;
}

void ForeachStatement::set_token(const Token& token) {
    this->token = token;
}
