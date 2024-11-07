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

std::string Expression::to_json() {
    return "unknown expression json";
}

std::string Expression::to_str() {
    return "unknown expression";
}
