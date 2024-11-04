#include "haard/semantic/semantic_analyzer.h"
#include "haard/log/logs.h"

using namespace haard;

SemanticAnalyzer::SemanticAnalyzer() {
    current_scope = new Scope();
}

SemanticAnalyzer::~SemanticAnalyzer() {

}

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
    SymbolKind kind = SYM_CLASS;
    std::string kind_as_str = "class";

    if (type->get_kind() == AST_CLASS) {
        kind = SYM_CLASS;
        kind_as_str = "class";
    } else if (type->get_kind() == AST_STRUCT) {
        kind = SYM_STRUCT;
        kind_as_str = "struct";
    } else if (type->get_kind() == AST_ENUM) {
        kind = SYM_ENUM;
        kind_as_str = "enum";
    } else if (type->get_kind() == AST_UNION) {
        kind = SYM_UNION;
        kind_as_str = "union";
    }

    auto symbols = current_scope->resolve_local(name);

    if (symbols.size() == 0) {
        current_scope->define(kind, name, type);
        log_info("Defining " + kind_as_str + " '" + name + "'");
    } else {
        for (auto sym : symbols) {
            Ast* other = sym->get_ast();

            Ast* g1 = other->get_child(AST_GENERICS);
            Ast* g2 = other->get_child(AST_GENERICS);

            if (g1 && g2) {
                if (g1->children_count() == g2->children_count()) {
                    log_error("User type '" + name + "' already defined");
                }
            } else if (g1 == nullptr && g2 == nullptr) {
                log_error("User type '" + name + "' already defined");
            } else {
                current_scope->define(kind, name, type);
                log_info("Defining " + kind_as_str + " '" + name + "'");
            }
        }
    }
}

void SemanticAnalyzer::declare_module_functions(Ast* mod) {
    auto functions = mod->get_children(AST_FUNCTION);

    for (auto f : functions) {
        declare_function(f);
    }
}

void SemanticAnalyzer::declare_function(Ast* f) {

}
