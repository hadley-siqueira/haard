#include "haard/ast/expressions/operators/unary/new.h"

using namespace haard;

New::New() {
    set_kind(AST_NEW);
}

New::New(Token& token, Expression* expression) {
    set_kind(AST_NEW);
    set_token(token);
}

New::~New() {
    for (auto expr : arguments) {
        delete expr;
    }
}

void New::add_argument(Expression* expression) {
    arguments.push_back(expression);
    expression->set_parent(this);
}

std::vector<Expression*>& New::get_arguments() {
    return arguments;
}

Token New::get_token() const {
    return token;
}

void New::set_token(const Token &token) {
    this->token = token;
}
