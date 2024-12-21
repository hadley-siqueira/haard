#include "haard/ast/expressions/operators/binary/call.h"

using namespace haard;

Call::Call() {
    set_kind(AST_CALL);
    set_expression(nullptr);
}

Call::~Call() {
    delete expression;

    for (auto expr : arguments) {
        delete expr;
    }
}

void Call::add_argument(Expression* expression) {
    arguments.push_back(expression);
    expression->set_parent(this);
}

std::vector<Expression*>& Call::get_arguments() {
    return arguments;
}

Expression* Call::get_expression() const {
    return expression;
}

void Call::set_expression(Expression* expression) {
    this->expression = expression;
}

Token Call::get_token() const {
    return token;
}

void Call::set_token(const Token& token) {
    this->token = token;
}
