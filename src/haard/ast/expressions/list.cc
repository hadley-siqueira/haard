#include "haard/ast/expressions/list.h"

using namespace haard;

List::List() {
    set_kind(AST_LIST);
}

List::~List() {
    for (auto s : expressions) {
        delete s;
    }
}

void List::add_expression(Expression* expression) {
    if (expression) {
        expressions.push_back(expression);
        expression->set_parent(this);
    }
}

std::vector<Expression*>& List::get_expressions() {
    return expressions;
}

const Token& List::get_token() const {
    return token;
}

void List::set_token(const Token& token) {
    this->token = token;
}
