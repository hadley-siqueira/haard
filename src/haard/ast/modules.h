#ifndef HAARD_AST_MODULES_H
#define HAARD_AST_MODULES_H

#include <unordered_map>

#include <haard/ast/module.h>

namespace haard {
    class Modules : public Ast {
    public:
        Modules();
        ~Modules();

    public:
        void add_module(Module* mod);

    private:
        std::unordered_map<std::string, Module*> modules;
    };
}

#endif
