#include <sstream>

#include "haard/ast/statements/statements.h"

using namespace haard;

Statements::Statements() {
    set_kind(AST_STATEMENTS);
}

Statements::~Statements() {
    for (auto s : statements) {
        delete s;
    }
}

void Statements::add_statement(Statement* statement) {
    if (statement) {
        statements.push_back(statement);
        statement->set_parent(this);
    }
}

std::vector<Statement*>& Statements::get_statements() {
    return statements;
}
