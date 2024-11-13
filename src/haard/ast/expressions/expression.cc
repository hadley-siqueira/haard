#include "haard/ast/expressions/expression.h"

using namespace haard;

Expression::Expression() {
    set_kind(AST_UNKNOWN);
    set_type(nullptr);
}

Expression::~Expression() {
    delete type;
}

Type* Expression::get_type() const {
    return type;
}

void Expression::set_type(Type* type) {
    this->type = type;
}
