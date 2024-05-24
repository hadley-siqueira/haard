#ifndef HAARD_MODULE_SEMANTIC_ANALYSER_H
#define HAARD_MODULE_SEMANTIC_ANALYSER_H

#include "ast/module.h"
#include "log/logger.h"

namespace haard {
    class ModuleSemanticAnalyser {
    public:
        void define_types(Module* module);
        void define_classes();
        void define_structs();
        void define_unions();
        void define_type(SymbolDescriptorKind kind, NamedTypeDescriptor* desc);
        void define_type_in_scope(SymbolDescriptorKind kind, NamedTypeDescriptor* desc);

        void check_for_type_redefinition(NamedTypeDescriptor* desc, Symbol* sym);

        Logger *get_logger() const;
        void set_logger(Logger *newLogger);

    private:
        Module* module;
        Logger* logger;

    };
}

#endif
