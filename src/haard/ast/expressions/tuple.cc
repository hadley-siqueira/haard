#include <sstream>

#include "haard/ast/expressions/tuple.h"

using namespace haard;

Tuple::Tuple() {
    set_kind(AST_TUPLE);
}

Tuple::~Tuple() {
    for (auto s : expressions) {
        delete s;
    }
}

void Tuple::add_expression(Expression* expression) {
    if (expression) {
        expressions.push_back(expression);
        expression->set_parent(this);
    }
}

std::vector<Expression*>& Tuple::get_expressions() {
    return expressions;
}

const Token& Tuple::get_token() const {
    return token;
}

void Tuple::set_token(const Token& token) {
    this->token = token;
}
