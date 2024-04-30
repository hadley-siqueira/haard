#ifndef HAARD_MODULES_CPP_GENERATOR_H
#define HAARD_MODULES_CPP_GENERATOR_H

#include "ast/modules.h"

namespace haard {
    class ModulesCppGenerator {
    public:
        void build(Modules* modules);
        void build_module(Module* module);
    };
}

#endif
