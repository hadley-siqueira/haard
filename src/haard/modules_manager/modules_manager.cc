#include "haard/modules_manager/modules_manager.h"

using namespace haard;

void ModulesManager::add_module(std::string& path, Ast* module) {
    modules[path] = module;
}

Ast* ModulesManager::get_module_by_path(std::string& path) {
    if (modules.count(path) > 0) {
        return modules.at(path);
    }

    return nullptr;
}

std::map<std::string, Ast*>& ModulesManager::get_modules() {
    return modules;
}
