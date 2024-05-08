#include <sstream>

#include "ast/named_type_descriptor.h"
#include "ast/module.h"
#include "ast/named_type.h"

using namespace haard;

NamedTypeDescriptor::NamedTypeDescriptor() {
    set_generics(nullptr);
    set_super_type(nullptr);
}

NamedTypeDescriptor::~NamedTypeDescriptor() {
    for (auto v : variables) {
        delete v;
    }

    for (auto f : functions) {
        delete f;
    }
}

void NamedTypeDescriptor::add_function(Function* function) {
    functions.push_back(function);
    function->set_named_type_descriptor(this);
}

void NamedTypeDescriptor::add_variable(Variable* variable) {
    variables.push_back(variable);
}

int NamedTypeDescriptor::functions_count() {
    return functions.size();
}

int NamedTypeDescriptor::variables_count() {
    return variables.size();
}

Function* NamedTypeDescriptor::get_function(int idx) {
    if (idx < functions_count()) {
        return functions[idx];
    }

    return nullptr;
}

Variable* NamedTypeDescriptor::get_variable(int idx) {
    if (idx < variables_count()) {
        return variables[idx];
    }

    return nullptr;
}

TypeList* NamedTypeDescriptor::get_generics() const {
    return generics;
}

void NamedTypeDescriptor::set_generics(TypeList* newGenerics) {
    generics = newGenerics;
}

Type* NamedTypeDescriptor::get_super_type() const {
    return super_type;
}

void NamedTypeDescriptor::set_super_type(Type* newSuper_type) {
    super_type = newSuper_type;
}

std::string NamedTypeDescriptor::get_qualified_name() {
    std::stringstream ss;

    ss << get_module()->get_qualified_path() << ".";
    ss << get_name().get_value();

    if (get_generics()) {
        ss << "<";

        for (int i = 0; i < generics->types_count(); ++i) {
            ss << "%0, ";
        }

        ss << ">";
    }

    return ss.str();
}
