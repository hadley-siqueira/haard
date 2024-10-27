#ifndef HAARD_MODULES_MANAGER_H
#define HAARD_MODULES_MANAGER_H

#include <map>
#include <string>

#include "haard/ast/ast.h"
#include "haard/ast/module.h"

namespace haard {
    class Modules {
    public:
        void add_module(std::string& path, Module* module);
        Module* get_module_by_path(std::string& path);
        std::map<std::string, Module*>& get_modules();

    private:
        std::map<std::string, Module*> modules;
    };
}

#endif
