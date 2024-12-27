#include "haard/ast/user_type.h"

using namespace haard;

UserType::UserType() {
    set_generics(nullptr);
    set_base_type(nullptr);
}

UserType::~UserType() {
    delete generics;
    delete base_type;

    for (auto v : variables) {
        delete v;
    }

    for (auto f : functions) {
        delete f;
    }
}

void UserType::add_variable(Variable* var) {
    if (var) {
        variables.push_back(var);
        var->set_parent(this);
    }
}

void UserType::add_function(Function* function) {
    if (function) {
        functions.push_back(function);
        function->set_parent(this);
    }
}

Generics *UserType::get_generics() const {
    return generics;
}

void UserType::set_generics(Generics* generics) {
    this->generics = generics;

    if (generics) {
        generics->set_parent(this);
    }
}

std::vector<Variable*>& UserType::get_variables() {
    return variables;
}

void UserType::set_variables(const std::vector<Variable*>& variables) {
    this->variables = variables;

    for (auto v : variables) {
        v->set_parent(this);
    }

}

std::vector<Function*>& UserType::get_functions() {
    return functions;
}

void UserType::set_functions(const std::vector<Function*>& functions) {
    this->functions = functions;

    for (auto f : functions) {
        f->set_parent(this);
    }
}

Token UserType::get_name() const {
    return name;
}

void UserType::set_name(const Token& token) {
    name = token;
}

Type* UserType::get_base_type() const {
    return base_type;
}

void UserType::set_base_type(Type* base_type) {
    this->base_type = base_type;

    if (base_type) {
        base_type->set_parent(this);
    }
}
