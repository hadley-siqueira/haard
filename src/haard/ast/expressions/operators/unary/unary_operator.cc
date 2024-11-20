#include "haard/ast/expressions/operators/unary/unary_operator.h"

using namespace haard;

UnaryOperator::UnaryOperator() {
    set_kind(AST_UNKNOWN);
    set_expression(nullptr);
}

UnaryOperator::~UnaryOperator() {
    delete expression;
}

Expression* UnaryOperator::get_expression() const {
    return expression;
}

void UnaryOperator::set_expression(Expression* expression) {
    if (expression) {
        this->expression = expression;
        expression->set_parent(this);
    }
}

const Token& UnaryOperator::get_token() const {
    return token;
}

void UnaryOperator::set_token(const Token& newToken) {
    token = newToken;
}
