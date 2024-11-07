#include <sstream>

#include "haard/ast/statements/while_statement.h"
#include "haard/utils/utils.h"

using namespace haard;

WhileStatement::WhileStatement() {
    set_kind(AST_WHILE);
}

WhileStatement::~WhileStatement() {
    delete statements;
    delete expression;
}

CompoundStatement* WhileStatement::get_statements() const {
    return statements;
}

void WhileStatement::set_statements(CompoundStatement* statements) {
    this->statements = statements;
}

Expression* WhileStatement::get_expression() const {
    return expression;
}

void WhileStatement::set_expression(Expression* expression) {
    this->expression = expression;
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
