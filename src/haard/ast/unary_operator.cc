#include <haard/ast/unary_operator.h>

using namespace haard;

UnaryOperator::UnaryOperator() {
    set_kind(AST_UNKNOWN);
    set_expression(nullptr);
}

UnaryOperator::UnaryOperator(AstKind kind, Expression* expression, const Token& token) {
    set_kind(kind);
    set_expression(expression);
    set_token(token);
}

UnaryOperator::~UnaryOperator() {
    delete expression;
}

void UnaryOperator::set_token(const Token& token) {
    this->token = token;
}

Token UnaryOperator::get_token() {
    return token;
}

void UnaryOperator::set_expression(Expression* expression) {
    if (expression) {
        this->expression = expression;
        expression->set_parent_node(this);
    }
}

Expression* UnaryOperator::get_expression() {
    return expression;
}

