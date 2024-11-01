#include "haard/semantic/semantic_analyzer.h"

using namespace haard;

void SemanticAnalyzer::process_module(Ast* mod) {
    declare_module_user_types(mod);
    declare_module_functions(mod);
}

void SemanticAnalyzer::declare_module_user_types(Ast* mod) {
    auto types = mod->get_children(AST_CLASS);

    for (auto t : types) {
        declare_user_type(t);
    }
}

void SemanticAnalyzer::declare_user_type(Ast* type) {
    std::string name = type->get_value();

    Symbol* sym = scope->resolve_local(name);

    if
}

void SemanticAnalyzer::declare_module_functions(Ast* mod) {
    auto functions = mod->get_children(AST_FUNCTION);

    for (auto f : functions) {
        declare_function(f);
    }
}

void SemanticAnalyzer::declare_function(Ast* f) {

}
