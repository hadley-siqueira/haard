#include <iostream>
#include <sstream>
#include <fstream>
#include "utils/utils.h"

#include "semantic/module_semantic_analyser.h"
#include "semantic/module_named_type_definer.h"
#include "semantic/module_function_definer.h"

using namespace haard;

void ModuleSemanticAnalyser::define_types(Module* module) {
    ModuleNamedTypeDefiner gen;

    gen.set_logger(get_logger());
    gen.build(module);
}

void ModuleSemanticAnalyser::define_functions(Module* module) {
    ModuleFunctionDefiner gen;

    gen.set_logger(get_logger());
    gen.build(module);
}

Logger* ModuleSemanticAnalyser::get_logger() const {
    return logger;
}

void ModuleSemanticAnalyser::set_logger(Logger* newLogger) {
    logger = newLogger;
}
