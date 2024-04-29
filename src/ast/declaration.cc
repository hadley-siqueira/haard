#include "ast/declaration.h"
#include "ast/module.h"

using namespace haard;

const Token& Declaration::get_name() const {
    return name;
}

void Declaration::set_name(const Token& newName) {
    name = newName;
}

Module* Declaration::get_module() const {
    return module;
}

void Declaration::set_module(Module* newModule) {
    module = newModule;
}
