#include <haard/ast/binary_operator.h>

using namespace haard;

BinaryOperator::BinaryOperator() {
    set_kind(AST_UNKNOWN);
    set_left(nullptr);
    set_right(nullptr);
}

BinaryOperator::BinaryOperator (
    AstKind kind, 
    const Token& token, 
    Expression* left, 
    Expression* right) {

    set_kind(kind);
    set_token(token);
    set_left(left);
    set_right(right);
}

BinaryOperator::~BinaryOperator() {
    delete left;
    delete right;
}

void BinaryOperator::set_token(const Token& token) {
    this->token = token;
}

Token BinaryOperator::get_token() {
    return token;
}

void BinaryOperator::set_left(Expression* expression) {
    left = expression;
}

Expression* BinaryOperator::get_left() {
    return left;
}

void BinaryOperator::set_right(Expression* expression) {
    right = expression;
}

Expression* BinaryOperator::get_right() {
    return right;
}
