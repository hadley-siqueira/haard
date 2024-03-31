#include "semantic/semantic_first_pass.h"

using namespace haard;

void SemanticFirstPass::build_modules(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module(modules->get_module(i));
    }
}

void SemanticFirstPass::build_module(Module* module) {
    build_module_functions(module);
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
/*        define_function(function);
        define(SYM_FUNCTION, name, function);
        return;*/
    }

    if (sym->is_function()) {

    }

    logger->error("can't define function " + name + ". Already defined ");
}
