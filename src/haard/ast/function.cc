#include "haard/ast/function.h"

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
    set_generics(nullptr);
    set_statements(nullptr);
}

Function::~Function() {
    delete return_type;
    delete generics;
    delete statements;

    for (auto p : parameters) {
        delete p;
    }
}

const Token& Function::get_name() const {
    return name;
}

void Function::set_name(const Token& name) {
    this->name = name;
}

Type* Function::get_return_type() const {
    return return_type;
}

void Function::set_return_type(Type* type) {
    this->return_type = type;
}

const std::vector<Variable*>& Function::get_parameters() const {
    return parameters;
}

void Function::set_parameters(const std::vector<Variable* >& parameters) {
    this->parameters = parameters;
}

Generics* Function::get_generics() const {
    return generics;
}

void Function::set_generics(Generics* generics) {
    this->generics = generics;

    if (generics) {
        generics->set_parent(this);
    }
}

Statements* Function::get_statements() const {
    return statements;
}

void Function::set_statements(Statements* statements) {
    this->statements = statements;

    if (statements) {
        statements->set_parent(this);
    }
}

void Function::add_parameter(Variable* parameter) {
    parameters.push_back(parameter);
    parameter->set_parent(this);
}
