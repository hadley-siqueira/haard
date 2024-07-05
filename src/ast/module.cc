#include "ast/module.h"
#include "utils/utils.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
    set_scope(new Scope());
}

Module::~Module() {
    for (auto d : declarations) {
        delete d;
    }

    delete scope;
}

void Module::add_import(Import* import) {
    imports.push_back(import);
    declarations.push_back(import);
}

int Module::imports_count() {
    return imports.size();
}

Import* Module::get_import(int idx) {
    return imports[idx];
}

Import* Module::get_import_with_alias(std::string alias) {
    for (int i = 0; i < imports_count(); ++i) {
        Import* imp = get_import(i);

        std::string imp_alias = imp->get_alias().get_value();

        if (alias == imp_alias) {
            return imp;
        }
    }

    return nullptr;
}

void Module::add_function(Function* function) {
    functions.push_back(function);
    declarations.push_back(function);
    function->set_module(this);
}

void Module::add_class(Class* klass) {
    classes.push_back(klass);
    declarations.push_back(klass);
    klass->set_module(this);
}

void Module::add_struct(Struct* s) {
    structs.push_back(s);
    declarations.push_back(s);
    s->set_module(this);
}

void Module::add_union(Union* u) {
    unions.push_back(u);
    declarations.push_back(u);
    u->set_module(this);
}

void Module::add_enum(Enum* e) {
    enums.push_back(e);
    declarations.push_back(e);
    e->set_module(this);
}

int Module::functions_count() {
    return functions.size();
}

int Module::classes_count() {
    return classes.size();
}

int Module::structs_count() {
    return structs.size();
}

int Module::unions_count() {
    return unions.size();
}

int Module::enums_count() {
    return enums.size();
}

Class* Module::get_class(int idx) {
    if (idx < classes_count()) {
        return classes[idx];
    }

    return nullptr;
}

Struct* Module::get_struct(int idx) {
    if (idx < structs_count()) {
        return structs[idx];
    }

    return nullptr;
}

Union* Module::get_union(int idx) {
    if (idx < unions_count()) {
        return unions[idx];
    }

    return nullptr;
}

Enum* Module::get_enum(int idx) {
    if (idx < enums_count()) {
        return enums[idx];
    }

    return nullptr;
}

Function* Module::get_function(int idx) {
    if (idx < functions_count()) {
        return functions[idx];
    }

    return nullptr;
}

int Module::declarations_count() {
    return declarations.size();
}

Declaration* Module::get_declaration(int idx) {
    return declarations[idx];
}

Scope* Module::get_scope() const {
    return scope;
}

void Module::set_scope(Scope* newScope) {
    scope = newScope;
}

std::string Module::get_qualified_path() {
    return relative_path;
}

std::string Module::get_cpp_namespace() {
    return join(split(relative_path, '.'), "::");
}

std::string Module::get_cpp_path() {
    return join(split(relative_path, '.'), "/") + ".cpp";
}

const std::string& Module::get_path() const {
    return path;
}

void Module::set_path(const std::string& newPath) {
    path = newPath;
}

const std::string& Module::get_relative_path() const {
    return relative_path;
}

void Module::set_relative_path(const std::string& newRelative_path) {
    relative_path = newRelative_path;
}
