#include "haard/ast/parameter.h"

using namespace haard::ast;

const std::string& Parameter::get_name() const {
    return name;
}

void Parameter::set_name(const std::string& name) {
    this->name = name;
}
