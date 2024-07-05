#include "ast/literals/tuple_literal.h"

using namespace haard;

TupleLiteral::TupleLiteral() {
    set_kind(AST_TUPLE);
    expressions = new ExpressionList();
}

TupleLiteral::~TupleLiteral() {
    delete expressions;
}

void TupleLiteral::add_expression(Expression* expr) {
    expressions->add_expression(expr);
}

ExpressionList* TupleLiteral::get_expressions() {
    return expressions;
}
