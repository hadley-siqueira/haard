#include "haard/ast/expressions/operators/binary/binary_operator.h"

using namespace haard;

BinaryOperator::BinaryOperator() {
    set_kind(AST_UNKNOWN);
    set_left(nullptr);
    set_right(nullptr);
}

BinaryOperator::~BinaryOperator() {
    delete left;
    delete right;
}

Expression* BinaryOperator::get_left() const {
    return left;
}

void BinaryOperator::set_left(Expression* expression) {
    this->left = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

Expression* BinaryOperator::get_right() const {
    return right;
}

void BinaryOperator::set_right(Expression* expression) {
    this->right = expression;

    if (expression) {
        expression->set_parent(this);
    }
}

const Token& BinaryOperator::get_token() const {
    return token;
}

void BinaryOperator::set_token(const Token& newToken) {
    token = newToken;
}
