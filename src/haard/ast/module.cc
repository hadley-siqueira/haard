#include <sstream>

#include "haard/ast/module.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
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
