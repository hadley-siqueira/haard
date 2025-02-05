#include "haard/ast/module.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
    set_symbol_table(new SymbolTable());
}

Module::~Module() {
    delete symbol_table;

    /*for (auto c : children) {
        delete c;
    }*/
}

void Module::add_import(Import* import) {
    imports.push_back(import);
    children.push_back(import);
    import->set_parent(this);
}

void Module::add_function(Function* function) {
    if (function) {
        functions.push_back(function);
        children.push_back(function);
        function->set_parent(this);
    }
}

void Module::add_class(Class* klass) {
    if (klass) {
        classes.push_back(klass);
        children.push_back(klass);
        klass->set_parent(this);
    }
}

const std::string& Module::get_path() const {
    return path;
}

void Module::set_path(const std::string &path) {
    this->path = path;
}

SymbolTable* Module::get_symbol_table() const {
    return symbol_table;
}

void Module::set_symbol_table(SymbolTable* symbol_table) {
    this->symbol_table = symbol_table;
}

const std::vector<Class*>& Module::get_classes() const {
    return classes;
}

void Module::set_classes(const std::vector<Class*>& classes) {
    this->classes = classes;
}

const std::vector<Function*>& Module::get_functions() const {
    return functions;
}

void Module::set_functions(const std::vector<Function*>& functions) {
    this->functions = functions;
}
