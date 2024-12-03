#include <sstream>

#include "haard/ast/statements/for_statement.h"

using namespace haard;

ForStatement::ForStatement() {
    set_kind(AST_FOR);
    set_test(nullptr);
    set_statements(nullptr);
}

ForStatement::~ForStatement() {
    delete statements;
    delete test;

    for (auto e : initialization) {
        delete e;
    }

    for (auto e : update) {
        delete e;
    }
}

Statements* ForStatement::get_statements() const {
    return statements;
}

void ForStatement::set_statements(Statements* statements) {
    this->statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

Expression* ForStatement::get_test() const {
    return test;
}

void ForStatement::set_test(Expression* expression) {
    this->test = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

const Token& ForStatement::get_token() const {
    return token;
}

void ForStatement::set_token(const Token& token) {
    this->token = token;
}

std::vector<Expression*>& ForStatement::get_initialization() {
    return initialization;
}

void ForStatement::set_initialization(const std::vector<Expression*>& initialization) {
    this->initialization = initialization;

    for (auto e : initialization) {
        e->set_parent(this);
    }
}

std::vector<Expression*>& ForStatement::get_update() {
    return update;
}

void ForStatement::set_update(const std::vector<Expression*>& update) {
    this->update = update;

    for (auto e : update) {
        e->set_parent(this);
    }
}
