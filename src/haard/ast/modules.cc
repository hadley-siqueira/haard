#include "haard/ast/modules.h"

using namespace haard;

void Modules::add_module(std::string& path, Module* module) {
    modules[path] = module;
}

Module* Modules::get_module_by_path(std::string& path) {
    if (modules.count(path) > 0) {
        return modules.at(path);
    }

    return nullptr;
}

std::map<std::string, Module*>& Modules::get_modules() {
    return modules;
}
