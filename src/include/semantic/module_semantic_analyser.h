#ifndef HAARD_MODULE_SEMANTIC_ANALYSER_H
#define HAARD_MODULE_SEMANTIC_ANALYSER_H

#include "ast/module.h"
#include "log/logger.h"
#include "semantic/semantic_pass.h"

namespace haard {
    class ModuleSemanticAnalyser : public SemanticPass {
    public:
        ModuleSemanticAnalyser();
        ~ModuleSemanticAnalyser();

    public:
        void define_types(Module* module);
        void define_functions(Module* module);

        Logger* get_logger() const;
        void set_logger(Logger *newLogger);

    private:
        Module* module;
        Logger* logger;
    };
}

#endif
