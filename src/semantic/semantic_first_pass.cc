#include <iostream>

#include "semantic/semantic_first_pass.h"

using namespace haard;

void SemanticFirstPass::build_modules(Modules* modules) {
    build_modules_function(modules);
}

void SemanticFirstPass::build_modules_function(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module_functions(modules->get_module(i));
    }
}

void SemanticFirstPass::build_module_functions(Module* module) {
    enter_scope(module->get_scope());

    for (int i = 0; i < module->functions_count(); ++i) {
        build_function(module->get_function(i));
    }

    leave_scope();
}

void SemanticFirstPass::build_function(Function* function) {
    std::string name = function->get_name().get_value();

    Symbol* sym = resolve(name);

    if (sym == nullptr) {
        define_function(function);
        return;
    }

    if (sym->is_function()) {
        Function* other = check_for_overloaded(sym, function);

        if (other != nullptr) {
            logger->error("Can't define function. Function already defined with same signature");
        } else {
            define_function(function);
        }
    } else {
        logger->error("Can't define function " + name + ". Already defined ");
    }
}

void SemanticFirstPass::define_function(Function* function) {
    std::string name = function->get_name().get_value();
    define(SYM_FUNCTION, name, function);
    logger->info("defining function " + name);
}

Function* SemanticFirstPass::check_for_overloaded(Symbol* sym, Function* function) {
    Function* other = nullptr;

    for (int i = 0; i < sym->descriptors_count(); ++i) {
        other = (Function*) sym->get_descriptor(i);

        if (false /*function->equals(other)*/) {
            return other;
        }
    }

    return nullptr;
}
