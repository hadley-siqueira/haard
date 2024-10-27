#include "haard/ast/variable.h"

using namespace haard;

Variable::Variable() {
    type = nullptr;
}

Variable::~Variable() {
    delete type;
}

const Token& Variable::get_name() const {
    return name;
}

void Variable::set_name(const Token& name) {
    this->name = name;
}

Type* Variable::get_type() const {
    return type;
}

void Variable::set_type(Type* type) {
    this->type = type;
}
