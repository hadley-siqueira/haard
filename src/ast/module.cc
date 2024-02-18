#include "ast/module.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
    for (auto d : declarations) {
        delete d;
    }
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

void Module::add_function(Function* function) {
    functions.push_back(function);
    declarations.push_back(function);
}

void Module::add_class(Class* klass) {
    classes.push_back(klass);
    declarations.push_back(klass);
}

void Module::add_struct(Struct* s) {
    structs.push_back(s);
    declarations.push_back(s);
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

