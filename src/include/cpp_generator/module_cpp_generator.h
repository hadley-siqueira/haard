#ifndef HAARD_MODULE_CPP_GENERATOR_H
#define HAARD_MODULE_CPP_GENERATOR_H

#include <sstream>
#include "ast/module.h"

namespace haard {
    class ModuleCppGenerator {
    public:
        void build(Module* module);
        void create_module_directory(Module* module);
        void build_module_header(Module* module);
        void build_module_cpp(Module* module);

        void build_module_classes(Module* module);

    private:
        std::stringstream cpp;
        std::stringstream header;
        std::string filepath;
    };
}

#endif
