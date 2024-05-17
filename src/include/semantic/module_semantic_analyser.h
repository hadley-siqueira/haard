#ifndef HAARD_MODULE_SEMANTIC_ANALYSER_H
#define HAARD_MODULE_SEMANTIC_ANALYSER_H

#include "ast/module.h"
#include "log/logger.h"

namespace haard {
    class ModuleSemanticAnalyser {
    public:
        void define_types(Module* module);
        void define_classes(Module* module);
        void define_class(Module* module, Class* klass);

        void define_structs(Module* module);
        void define_struct(Module* module, Struct* s);

    private:
        Logger* logger;

    };
}

#endif
