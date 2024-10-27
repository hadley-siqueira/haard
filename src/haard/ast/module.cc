#include "haard/ast/module.h"

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
    for (auto def : defs) {
        delete def;
    }
}

void Module::add_import(Import* import) {
    imports.push_back(import);
    defs.push_back(import);
    import->set_parent(this);
}

const std::vector<Import*>& Module::get_imports() {
    return imports;
}

void Module::add_function(Function* function) {
    functions.push_back(function);
    defs.push_back(function);
    function->set_module(this);
}

const std::vector<Ast*>& Module::get_defs() {
    return defs;
}
