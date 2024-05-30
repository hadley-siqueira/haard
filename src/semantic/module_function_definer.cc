#include "semantic/module_function_definer.h"

using namespace haard;

void ModuleFunctionDefiner::build(Module* module) {
    this->module = module;

    for (int i = 0; i < module->functions_count(); ++i) {
        define_function(module->get_function(i));
    }
}

void ModuleFunctionDefiner::define_function(Function* function) {
    std::string name = function->get_name().get_value();
    Scope* scope = module->get_scope();
    Symbol* sym = scope->resolve_local(name);

    if (sym == nullptr) {
        define_function_in_scope(function);
        return;
    }

    for (int i = 0; i < sym->descriptors_count(); ++i) {
        SymbolDescriptor* sd = sym->get_descriptor(i);

        switch (sd->get_kind()) {
        case SYM_FUNCTION:
            check_for_function_redefinition(function, (Function*) sd->get_descriptor());
            break;
        }
    }
}

void ModuleFunctionDefiner::define_function_in_scope(Function* function) {
    module->get_scope()->define(SYM_FUNCTION, function->get_name().get_value(), function);
    logger->info("defining function " + std::string(function->get_name().get_value()));
}

void ModuleFunctionDefiner::check_for_function_redefinition(Function* f1, Function* f2) {
    TypeList* g1 = f1->get_generics();
    TypeList* g2 = f2->get_generics();
    int np1 = f1->parameters_count();
    int np2 = f2->parameters_count();

    if (g1 == nullptr && g2 == nullptr) {

    }

    if (np1 == np2) {
        for (int i = 0; i < np1; ++i) {
            Type* t1 = f1->get_parameter(i)->get_type();
            Type* t2 = f2->get_parameter(i)->get_type();

            if (t1->equals(t2)) {

            }
        }
    }
}
