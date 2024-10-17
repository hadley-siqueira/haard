#ifndef HAARD_MODULES_MANAGER_H
#define HAARD_MODULES_MANAGER_H

#include <map>
#include <string>

#include "haard/ast/ast.h"

namespace haard {
    class ModulesManager {
    public:
        void add_module(std::string& path, Ast* module);
        Ast* get_module_by_path(std::string& path);
        std::map<std::string, Ast*>& get_modules();

    private:
        std::map<std::string, Ast*> modules;
    };
}

#endif
