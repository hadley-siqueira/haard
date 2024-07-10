#include "haard/ast/function.h"

using namespace haard::ast;

void Function::add_parameter(Parameter* parameter) {
    parameters.push_back(parameter);
}

Parameter* Function::get_parameter(size_t idx) {
    return parameters[idx];
}

size_t Function::parameters_count() {
    return parameters.size();
}

const std::string &Function::get_name() const {
    return name;
}

void Function::set_name(const std::string &name) {
    this->name = name;
}
