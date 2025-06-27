#include <haard/ast/modules.h>

using namespace haard;

Modules::Modules() {
    set_kind(AST_MODULES);
}

Modules::~Modules() {
    for (auto obj : modules) {
        delete obj.second;
    }
}

void Modules::add_module(Module* mod) {
    auto path = mod->get_path();

    if (modules.count(path) == 0) {
        modules[path] = mod;
    }
}


