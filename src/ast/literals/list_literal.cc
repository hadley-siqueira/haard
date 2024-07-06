#include "ast/literals/list_literal.h"

using namespace haard;

ListLiteral::ListLiteral() {
    set_kind(AST_LIST);
    expressions = new ExpressionList();
}

ListLiteral::~ListLiteral() {
    delete expressions;
}

void ListLiteral::add_expression(Expression* expr) {
    expressions->add_expression(expr);
}

ExpressionList* ListLiteral::get_expressions() const {
    return expressions;
}

void ListLiteral::set_expressions(ExpressionList* newExpressions) {
    expressions = newExpressions;
}
