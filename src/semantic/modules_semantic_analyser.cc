#include "semantic/modules_semantic_analyser.h"
#include "semantic/module_semantic_analyser.h"

using namespace haard;

void ModulesSemanticAnalyser::build(Modules* modules) {
    logger->info("Starting semantic analysis...");

    define_types(modules);
}

void ModulesSemanticAnalyser::define_types(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        ModuleSemanticAnalyser gen;

        gen.define_types(modules->get_module(i));
    }
}

Logger* ModulesSemanticAnalyser::get_logger() const {
    return logger;
}

void ModulesSemanticAnalyser::set_logger(Logger* newLogger) {
    logger = newLogger;
}

