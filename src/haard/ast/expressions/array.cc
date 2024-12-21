#include "haard/ast/expressions/array.h"

using namespace haard;

Array::Array() {
    set_kind(AST_ARRAY);
}

Array::~Array() {
    for (auto s : expressions) {
        delete s;
    }
}

void Array::add_expression(Expression* expression) {
    if (expression) {
        expressions.push_back(expression);
        expression->set_parent(this);
    }
}

std::vector<Expression*>& Array::get_expressions() {
    return expressions;
}

const Token& Array::get_token() const {
    return token;
}

void Array::set_token(const Token& token) {
    this->token = token;
}
