#include <sstream>

#include "haard/ast/function.h"

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
    set_generics(nullptr);
}

Function::~Function() {
    delete return_type;
    delete generics;

    for (auto p : parameters) {
        delete p;
    }
}

const Token& Function::get_name() const {
    return name;
}

void Function::set_name(const Token& newName) {
    this->name = newName;
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

const Generics* Function::get_generics() const {
    return generics;
}

void Function::set_generics(Generics* generics) {
    this->generics = generics;
}

Ast* Function::get_statements() const {
    return statements;
}

void Function::set_statements(Ast* statements) {
    this->statements = statements;
}

std::string Function::to_json() {
    return "function json";
}

std::string Function::to_str() {
    std::stringstream ss;

    ss << "def " << name.get_value();

    if (generics) {
        ss << generics->to_str();
    }

    ss << " : " << return_type->to_str() << "\n";

    if (parameters.size() > 0) {
        for (auto p : parameters) {
            ss << "    " << p->to_str();
            ss << "\n";
        }

        ss << "\n";
    }

    return ss.str();
}
