#include "ast/literals/array_literal.h"

using namespace haard;

ArrayLiteral::ArrayLiteral() {
    set_kind(AST_ARRAY);
    expressions = new ExpressionList();
}

ArrayLiteral::~ArrayLiteral() {
    delete expressions;
}

void ArrayLiteral::add_expression(Expression* expr) {
    expressions->add_expression(expr);
}

ExpressionList* ArrayLiteral::get_expressions() const {
    return expressions;
}

void ArrayLiteral::set_expressions(ExpressionList* newExpressions) {
    expressions = newExpressions;
}
