#include "ast/literals/hash_literal.h"

using namespace haard;

HashLiteral::HashLiteral() {
    set_kind(AST_HASH);
    expressions = new ExpressionList();
}

HashLiteral::~HashLiteral() {
    delete expressions;
}

void HashLiteral::add_expression(Expression* expr) {
    expressions->add_expression(expr);
}

ExpressionList* HashLiteral::get_expressions() const {
    return expressions;
}

void HashLiteral::set_expressions(ExpressionList* newExpressions) {
    expressions = newExpressions;
}
