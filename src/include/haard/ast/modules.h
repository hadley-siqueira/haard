#ifndef HAARD_MODULES_H
#define HAARD_MODULES_H

#include <map>
#include <string>

#include "haard/ast/ast.h"
#include "haard/ast/module.h"

namespace haard {
    class Modules : public Ast {
    public:
        void add_module(std::string& path, Module* module);
        Module* get_module_by_path(std::string& path);
        std::map<std::string, Module*>& get_modules();

        virtual std::string to_json();

    private:
        std::map<std::string, Module*> modules;
    };
}

#endif
