#ifndef HAARD_MODULES_SEMANTIC_ANALYSER_H
#define HAARD_MODULES_SEMANTIC_ANALYSER_H

#include "ast/modules.h"
#include "log/logger.h"

namespace haard {
    class ModulesSemanticAnalyser {
    public:
        void build(Modules* modules);
        void define_types(Modules* modules);
        void define_functions(Modules* modules);

        Logger* get_logger() const;
        void set_logger(Logger* newLogger);

    private:
        Logger* logger;

    };
}

#endif
