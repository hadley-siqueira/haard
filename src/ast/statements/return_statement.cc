#include "ast/statements/return_statement.h"

using namespace haard;

ReturnStatement::ReturnStatement() {
    set_kind(STMT_RETURN);
    set_expression(nullptr);
}

ReturnStatement::~ReturnStatement() {
    delete expression;
}

Expression* ReturnStatement::get_expression() {
    return expression;
}

void ReturnStatement::set_expression(Expression* expression) {
    this->expression = expression;
}
