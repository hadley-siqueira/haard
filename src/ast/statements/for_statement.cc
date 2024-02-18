#include "ast/statements/for_statement.h"

using namespace haard;

ForStatement::ForStatement() {
    set_kind(STMT_FOR);
    set_init(nullptr);
    set_test(nullptr);
    set_update(nullptr);
    set_range(nullptr);
}

ForStatement::~ForStatement() {
    delete test;
    delete init;
    delete update;
    delete range;
    delete statements;
}

Expression* ForStatement::get_test() const {
    return test;
}

void ForStatement::set_test(Expression* newTest) {
    test = newTest;
}

ExpressionList* ForStatement::get_init() const {
    return init;
}

void ForStatement::set_init(ExpressionList *newInit) {
    init = newInit;
}

ExpressionList* ForStatement::get_update() const {
    return update;
}

void ForStatement::set_update(ExpressionList *newUpdate) {
    update = newUpdate;
}

Expression* ForStatement::get_range() const {
    return range;
}

void ForStatement::set_range(Expression* newRange) {
    range = newRange;
}

CompoundStatement* ForStatement::get_statements() const {
    return statements;
}

void ForStatement::set_statements(CompoundStatement* newStatements) {
    statements = newStatements;
}

bool ForStatement::is_foreach() {
    return range != nullptr;
}

