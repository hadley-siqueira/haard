#include <haard/ast/module.h>

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
    for (auto i : imports) {
        delete i;
    }

    for (auto v : variables) {
        delete v;
    }
}

void Module::add_import(Import* node) {
    if (node) {
        imports.push_back(node);
        node->set_parent_node(this);
    }
}

void Module::add_variable(Variable* node) {
    if (node) {
        variables.push_back(node);
        node->set_parent_node(this);
    }
}

void Module::add_function(Function* node) {
    if (node) {
        functions.push_back(node);
        node->set_parent_node(this);
    }
}
