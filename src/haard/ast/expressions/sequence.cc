#include <sstream>

#include "haard/ast/expressions/sequence.h"

using namespace haard;

Sequence::Sequence() {
    set_kind(AST_SEQUENCE);
}

Sequence::~Sequence() {
    for (auto s : expressions) {
        delete s;
    }
}

void Sequence::add_expression(Expression* expression) {
    if (expression) {
        expressions.push_back(expression);
        expression->set_parent(this);
    }
}

std::vector<Expression*>& Sequence::get_expressions() {
    return expressions;
}

const Token& Sequence::get_token() const {
    return token;
}

void Sequence::set_token(const Token& token) {
    this->token = token;
}
