#include <sstream>

#include "haard/ast/types/named_type.h"

using namespace haard;

NamedType::NamedType() {
    set_kind(AST_TYPE_NAMED);
    set_name_expression(nullptr);
}

NamedType::NamedType(Expression* expression) {
    set_kind(AST_TYPE_NAMED);
    set_name_expression(expression);
}

NamedType::~NamedType() {
    delete name_expression;
}

Expression* NamedType::get_name_expression() const {
    return name_expression;
}

void NamedType::set_name_expression(Expression* expression) {
    if (expression) {
        this->name_expression = expression;
        expression->set_parent(this);
    }
}

bool NamedType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (other->get_kind() != get_kind()) {
        return false;
    }

    NamedType* o2 = (NamedType*) other;

    return false;
}
