#include "ast/sequence.h"

using namespace haard;

Sequence::Sequence() {
    set_kind(AST_SEQUENCE);
    expressions = new ExpressionList();
}

Sequence::~Sequence() {
    delete expressions;
}

void Sequence::add_expression(Expression* expr) {
    expressions->add_expression(expr);
}

ExpressionList* Sequence::get_expressions() {
    return expressions;
}
