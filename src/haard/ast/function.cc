#include "haard/ast/function.h"

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
}

Function::~Function() {
    for (auto v : variables) {
        delete v;
    }
}

void Function::add_parameter(Variable* parameter) {
    parameters.push_back(parameter);
    add_variable(parameter);
}

void Function::add_variable(Variable* var) {
    variables.push_back(var);
}

Type* Function::get_return_type() const {
    return return_type;
}

void Function::set_return_type(Type* type) {
    return_type = type;
}

Module* Function::get_module() const {
    return module;
}

void Function::set_module(Module* module) {
    this->module = module;
}

const Token& Function::get_name() const {
    return name;
}

void Function::set_name(const Token& name) {
    this->name = name;
}
