#include "haard/ast/expressions/hash.h"

using namespace haard;

Hash::Hash() {
    set_kind(AST_HASH);
}

Hash::~Hash() {
    for (auto expr : expressions) {
        delete expr;
    }
}

void Hash::add_expression(Expression* expression) {
    if (expression) {
        expressions.push_back(expression);
        expression->set_parent(this);
    }
}

std::vector<Expression*>& Hash::get_expressions() {
    return expressions;
}

const Token& Hash::get_token() const {
    return token;
}

void Hash::set_token(const Token& token) {
    this->token = token;
}
