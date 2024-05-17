#include "semantic/module_semantic_analyser.h"

using namespace haard;

void ModuleSemanticAnalyser::define_types(Module* module) {
    define_classes(module);
    define_structs(module);
}

void ModuleSemanticAnalyser::define_classes(Module* module) {
    if (module->classes_count() == 0) {
        return;
    }

    logger->info("defining classes for module " + module->get_path());

    for (int i = 0; i < module->classes_count(); ++i) {
        define_class(module, module->get_class(i));
    }
}

void ModuleSemanticAnalyser::define_class(Module* module, Class* klass) {
    std::string name = klass->get_name().get_value();
    Scope* scope = module->get_scope();
    Symbol* sym = scope->resolve_local(name);

    if (sym == nullptr) {
        scope->define(SYM_CLASS, name, klass);
        logger->info("defining class " + klass->get_qualified_name());
        return;
    }

    for (int i = 0; i < sym->descriptors_count(); ++i) {
        SymbolDescriptor* sd = sym->get_descriptor(i);
        NamedTypeDescriptor* other = (Class*) sd->get_descriptor();

        switch (sd->get_kind()) {
        case SYM_CLASS:
        case SYM_ENUM:
        case SYM_UNION:
        case SYM_STRUCT:
        case SYM_INTERFACE:
            if (klass->get_generics() && other->get_generics()) {
                int n1 = klass->get_generics()->types_count();
                int n2 = other->get_generics()->types_count();

                if (n1 == n2) {
                    logger->error(name + " already defined in here");
                }
            } else if (klass->get_generics() == nullptr && other->get_generics() == nullptr) {
                logger->error(name + " already defined in here");
            }

            break;

        case SYM_FUNCTION:
            logger->error(name + " already defined in here");
            break;
        }
    }
}

void ModuleSemanticAnalyser::define_structs(Module* module) {
    if (module->structs_count() == 0) {
        return;
    }

    logger->info("defining structs for module " + module->get_path());

    for (int i = 0; i < module->classes_count(); ++i) {
        define_class(module, module->get_class(i));
    }
}
