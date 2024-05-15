#include "ast/function.h"
#include "scope/scope.h"
#include "ast/named_type_descriptor.h"
#include "ast/module.h"

using namespace haard;

Function::Function() {
    set_kind(AST_FUNCTION);
    set_generics(nullptr);
    set_return_type(nullptr);
    set_statements(nullptr);
    set_scope(new Scope());
    set_named_type_descriptor(nullptr);
}

Function::~Function() {
    for (auto p : parameters) {
        delete p;
    }

    for (auto v : variables) {
        delete v;
    }

    delete scope;
    delete statements;
}

void Function::add_parameter(Variable* param) {
    parameters.push_back(param);
}

int Function::parameters_count() {
    return parameters.size();
}

Variable* Function::get_parameter(int idx) {
    return parameters[idx];
}

void Function::add_variable(Variable* var) {
    variables.push_back(var);
}

int Function::variables_count() {
    return variables.size();
}

Variable* Function::get_variable(int idx) {
    return variables[idx];
}

Type* Function::get_return_type() const {
    return return_type;
}

void Function::set_return_type(Type* newReturn_type) {
    return_type = newReturn_type;
}

Type* Function::get_self_type() {
    return self_type;
}

void Function::set_self_type(Type* type) {
    self_type = type;
}

TypeList* Function::get_generics() const {
    return generics;
}

void Function::set_generics(TypeList* newGenerics) {
    generics = newGenerics;
}

CompoundStatement* Function::get_statements() const {
    return statements;
}

void Function::set_statements(CompoundStatement* newStatements) {
    statements = newStatements;
}

Scope* Function::get_scope() const {
    return scope;
}

void Function::set_scope(Scope* newScope) {
    scope = newScope;
}

bool Function::is_method() {
    return get_named_type_descriptor() != nullptr;
}

bool Function::is_constructor() {
    std::string name = get_name().get_value();

    return is_method() && name == "init";
}

bool Function::is_destructor() {
    std::string name = get_name().get_value();

    return is_method() && name == "destroy";
}

NamedTypeDescriptor* Function::get_named_type_descriptor() const {
    return named_type_descriptor;
}

void Function::set_named_type_descriptor(NamedTypeDescriptor* newNamed_type_descriptor) {
    named_type_descriptor = newNamed_type_descriptor;
}

std::string Function::get_cpp_namespace() {
    std::string ns;

    if (is_method()) {
        ns = named_type_descriptor->get_cpp_namespace();
    } else {
        ns = get_module()->get_cpp_namespace();
    }

    ns += "::";
    ns += get_name().get_value();
    return ns;
}
