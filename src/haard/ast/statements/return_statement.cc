#include "haard/ast/statements/return_statement.h"

using namespace haard;

ReturnStatement::ReturnStatement(Token& token) {
    set_kind(AST_RETURN);
    set_token(token);
    set_expression(nullptr);
}

ReturnStatement::ReturnStatement(Expression* expression) {
    set_kind(AST_RETURN);
    set_expression(expression);
}

ReturnStatement::~ReturnStatement() {
    delete expression;
}

Expression* ReturnStatement::get_expression() const {
    return expression;
}

void ReturnStatement::set_expression(Expression* expression) {
    this->expression = expression;
}

const Token& ReturnStatement::get_token() const {
    return token;
}

void ReturnStatement::set_token(const Token& token) {
    this->token = token;
}
