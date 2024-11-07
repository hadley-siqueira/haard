#include <sstream>

#include "haard/ast/statements/while_statement.h"
#include "haard/utils/utils.h"

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

std::string WhileStatement::to_json() {
    return "while json";
}

std::string WhileStatement::to_str() {
    std::stringstream ss;

    ss << "while " << expression->to_str() << ":\n";
    ss << indent(statements->to_str());

    return ss.str();
}

const Token& WhileStatement::get_token() const {
    return token;
}

void WhileStatement::set_token(const Token& token) {
    this->token = token;
}
