#include "ast/modules.h"
#include <iostream>

using namespace haard;

Modules::Modules() {

}

Modules::~Modules() {
    for (int i = 0; i < modules_count(); ++i) {
        delete modules[i];
    }
}

void Modules::add_module(std::string path, Module* module) {
    modules_map[path] = module;
    modules.push_back(module);
}

Module* Modules::get_module(std::string path) {
    if (has_module(path)) {
        return modules_map[path];
    }

    return nullptr;
}

Module* Modules::get_module(int i) {
    if (i < modules.size()) {
        return modules[i];
    }

    return nullptr;
}

bool Modules::has_module(std::string path) {
    return modules_map.count(path) > 0;
}

int Modules::modules_count() {
    return modules.size();
}
