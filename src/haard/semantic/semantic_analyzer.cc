#include "haard/semantic/semantic_analyzer.h"

using namespace haard;

void SemanticAnalyzer::analyze_module(Module* module) {
    define_module_classes(module);
}

void SemanticAnalyzer::define_module_classes(Module* module) {
    std::string name;

    for (auto child : module->get_children()) {
        name = get_name(child);

        Symbol* sym = scope->resolve(name);

        if (sym == nullptr) {

        } else {
            for (auto )
        }
    }
}

std::string SemanticAnalyzer::get_name(AstNode* node) {
    Function* f;
    UserType* u;

    if (node == nullptr) {
        return "";
    }

    switch (node->get_kind()) {
    case AST_FUNCTION:
        f = (Function*) node;
        return f->get_name().get_value();

    case AST_CLASS:
    case AST_STRUCT:
    case AST_ENUM:
    case AST_UNION:
        u = (UserType*) node;
        return u->get_name().get_value();

    default:
        break;
    }

    return "";
}
