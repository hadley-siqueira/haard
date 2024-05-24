#ifndef HAARD_MODULE_NAMED_TYPE_DEFINER_H
#define HAARD_MODULE_NAMED_TYPE_DEFINER_H

#include "ast/module.h"
#include "log/logger.h"

namespace haard {
    class ModuleNamedTypeDefiner {
    public:
        void build(Module* module);

        Logger* get_logger() const;
        void set_logger(Logger *newLogger);

    private:
        void define_types();
        void define_classes();
        void define_structs();
        void define_unions();
        void define_enums();
        void define_interfaces();

        void define_type(SymbolDescriptorKind kind, NamedTypeDescriptor* desc);
        void define_type_in_scope(SymbolDescriptorKind kind, NamedTypeDescriptor* desc);
        NamedTypeDescriptor* check_for_type_redefinition(NamedTypeDescriptor* desc, Symbol* sym);

    private:
        std::string error_type_redefinition(NamedTypeDescriptor* c1, NamedTypeDescriptor* c2);
        std::string get_kind_as_str(NamedTypeDescriptor* n);

    private:
        Module* module;
        Logger* logger;
    };
}

#endif
