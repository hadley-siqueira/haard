#include "ast/compound_statement.h"
#include "scope/scope.h"

using namespace haard;

CompoundStatement::CompoundStatement() {
    set_kind(STMT_COMPOUND);
    set_scope(new Scope());
}

CompoundStatement::~CompoundStatement() {
    for (auto s : statements) {
        delete s;
    }

    delete scope;
}

void CompoundStatement::add_statement(Statement* stmt) {
    statements.push_back(stmt);
}

int CompoundStatement::statements_count() {
    return statements.size();
}

Statement* CompoundStatement::get_statement(int idx) {
    return statements[idx];
}

Scope* CompoundStatement::get_scope() const {
    return scope;
}

void CompoundStatement::set_scope(Scope* newScope) {
    scope = newScope;
}
