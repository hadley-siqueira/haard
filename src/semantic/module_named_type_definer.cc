#include <sstream>

#include "semantic/module_named_type_definer.h"
#include "utils/utils.h"
#include "ast/types/named_type.h"

using namespace haard;

void ModuleNamedTypeDefiner::build(Module* module) {
    this->module = module;
    define_types();
}

void ModuleNamedTypeDefiner::define_types() {
    define_classes();
    define_structs();
    define_unions();
    define_enums();
    //define_interfaces();
}

void ModuleNamedTypeDefiner::define_classes() {
    if (module->classes_count() == 0) {
        return;
    }

    logger->info("defining classes for module " + module->get_path());

    for (int i = 0; i < module->classes_count(); ++i) {
        define_type(SYM_CLASS, module->get_class(i));
    }
}

void ModuleNamedTypeDefiner::define_structs() {
    if (module->structs_count() == 0) {
        return;
    }

    logger->info("defining structs for module " + module->get_path());

    for (int i = 0; i < module->structs_count(); ++i) {
        define_type(SYM_STRUCT, module->get_struct(i));
    }
}

void ModuleNamedTypeDefiner::define_unions() {
    if (module->unions_count() == 0) {
        return;
    }

    logger->info("defining unions for module " + module->get_path());

    for (int i = 0; i < module->unions_count(); ++i) {
        define_type(SYM_UNION, module->get_union(i));
    }
}

void ModuleNamedTypeDefiner::define_enums() {
    if (module->enums_count() == 0) {
        return;
    }

    logger->info("defining enums for module " + module->get_path());

    for (int i = 0; i < module->enums_count(); ++i) {
        define_type(SYM_ENUM, module->get_enum(i));
    }
}

void ModuleNamedTypeDefiner::define_type(SymbolDescriptorKind kind, NamedTypeDescriptor* desc) {
    std::string name = desc->get_name().get_value();
    Scope* scope = module->get_scope();
    Symbol* sym = scope->resolve_local(name);

    if (sym == nullptr) {
        define_type_in_scope(kind, desc);
        return;
    }

    NamedTypeDescriptor* other = check_for_type_redefinition(desc, sym);

    if (other != nullptr) {
        logger->error(error_type_redefinition(desc, other));
    } else {
        define_type_in_scope(kind, desc);
    }
}

void ModuleNamedTypeDefiner::define_type_in_scope(SymbolDescriptorKind kind, NamedTypeDescriptor* desc) {
    module->get_scope()->define(kind, desc->get_name().get_value(), desc);
    logger->info("defining " + get_kind_as_str(desc) + " " + desc->get_qualified_name());
}

bool same_generics(TypeList* t1, TypeList* t2) {
    if (t1 && t2) {
        int n1 = t1->types_count();
        int n2 = t2->types_count();

        if (n1 == n2) {
            return true;
        }
    } else if (t1 == nullptr && t2 == nullptr) {
        return true;
    }

    return false;
}

NamedTypeDescriptor* ModuleNamedTypeDefiner::check_for_type_redefinition(NamedTypeDescriptor* desc, Symbol* sym) {
    for (int i = 0; i < sym->descriptors_count(); ++i) {
        SymbolDescriptor* sd = sym->get_descriptor(i);
        NamedTypeDescriptor* other = (NamedTypeDescriptor*) sd->get_descriptor();

        if (sd->is_named_type()) {
            if (same_generics(desc->get_generics(), other->get_generics())) {
                return other;
            }
        }
    }

    return nullptr;
}

std::string ModuleNamedTypeDefiner::error_type_redefinition(NamedTypeDescriptor* c1, NamedTypeDescriptor* c2) {
    std::stringstream ss;
    std::string name = c1->get_name().get_value();
    int line1 = c1->get_name().get_line();
    int line2 = c2->get_name().get_line();
    int column1 = c1->get_name().get_column();
    int column2 = c2->get_name().get_column();
    std::string path1 = c1->get_module()->get_path();
    std::string path2 = c2->get_module()->get_path();
    std::string kind1 = get_kind_as_str(c1);
    std::string kind2 = get_kind_as_str(c2);

    if (c1->get_generics()) {
        int i;
        NamedType* n;
        name += '<';

        for (i = 0; i < c1->get_generics()->types_count() - 1; ++i) {
            n = (NamedType*) c1->get_generics()->get_type(i);
            name += n->get_identifier()->get_name().get_value();
            name += ", ";
        }

        n = (NamedType*) c1->get_generics()->get_type(i);
        name += n->get_identifier()->get_name().get_value();
        name += '>';
    }

    ss << "can't define <white>'" << kind1 << " " << name << "'<normal> on line " << line1 << " because it is already defined as <white>'" << kind2 << " " << name << "'<normal> on line " << line2 << '\n';
    ss << build_message(path1, line1, column1, "tried to define here") << "\n\n";
    ss << build_message(path2, line2, column2, "but is also defined here") << '\n';

    return ss.str();
}

std::string ModuleNamedTypeDefiner::get_kind_as_str(NamedTypeDescriptor* n) {
    switch (n->get_kind()) {
    case AST_CLASS:
        return "class";

    case AST_ENUM:
        return "enum";

    case AST_UNION:
        return "union";

    case AST_STRUCT:
        return "struct";
    }

    return "unknown";
}

Logger* ModuleNamedTypeDefiner::get_logger() const {
    return logger;
}

void ModuleNamedTypeDefiner::set_logger(Logger* newLogger) {
    logger = newLogger;
}
