#include "ast/expression.h"

using namespace haard;

Expression::Expression() {
    set_kind(AST_EXPRESSION_UNKNOWN);
    set_type(nullptr);
}

Type* Expression::get_type() const {
    return type;
}

void Expression::set_type(Type* newType) {
    type = newType;
}
