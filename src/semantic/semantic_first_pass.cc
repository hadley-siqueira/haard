#include "semantic/semantic_first_pass.h"

using namespace haard;

void SemanticFirstPass::build_modules(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module(modules->get_module(i));
    }
}

void SemanticFirstPass::build_module(Module* module) {
    enter_scope(module->get_scope());

    build_module_functions(module);

    leave_scope();
}

void SemanticFirstPass::build_module_functions(Module* module) {
    for (int i = 0; i < module->functions_count(); ++i) {
        build_function(module->get_function(i));
    }
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
    }

    logger->error("Can't define function " + name + ". Already defined ");
}

void SemanticFirstPass::define_function(Function* function) {
    define(SYM_FUNCTION, function->get_name().get_value(), function);
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
