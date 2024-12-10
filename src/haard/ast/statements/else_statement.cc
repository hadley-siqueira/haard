#include <sstream>

#include "haard/ast/statements/else_statement.h"

using namespace haard;

ElseStatement::ElseStatement() {
    set_kind(AST_ELSE);
    set_statements(nullptr);
}

ElseStatement::~ElseStatement() {
    delete statements;
}

Statements* ElseStatement::get_statements() const {
    return statements;
}

void ElseStatement::set_statements(Statements* statements) {
    this->statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

const Token& ElseStatement::get_token() const {
    return token;
}

void ElseStatement::set_token(const Token& token) {
    this->token = token;
}
