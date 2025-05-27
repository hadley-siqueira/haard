#include <haard/ast/module.h>

using namespace haard;

Module::Module() {
    set_kind(AST_MODULE);
}

Module::~Module() {
    for (auto i : imports) {
        delete i;
    }
}

void Module::add_import(Import* node) {
    if (node) {
        imports.push_back(node);
    }
}
