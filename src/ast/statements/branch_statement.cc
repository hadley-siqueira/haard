#include "ast/statements/branch_statement.h"

using namespace haard;

BranchStatement::BranchStatement(int kind) {
    set_kind(kind);
    set_condition(nullptr);
    set_true_statements(nullptr);
    set_false_statements(nullptr);
}

BranchStatement::~BranchStatement() {
    delete true_statements;
    delete false_statements;
    delete condition;
}

Expression* BranchStatement::get_condition() const {
    return condition;
}

void BranchStatement::set_condition(Expression* newCondition) {
    condition = newCondition;
}

Statement* BranchStatement::get_true_statements() const {
    return true_statements;
}

void BranchStatement::set_true_statements(Statement* newTrue_statements) {
    true_statements = newTrue_statements;
}

Statement* BranchStatement::get_false_statements() const {
    return false_statements;
}

void BranchStatement::set_false_statements(Statement* newFalse_statements) {
    false_statements = newFalse_statements;
}
