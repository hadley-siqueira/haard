#include <sstream>

#include "haard/semantic/semantic_analyzer.h"
#include "haard/log/logs.h"
#include "haard/utils/utils.h"
#include "haard/ast/ast.h"

using namespace haard;

SemanticAnalyzer::SemanticAnalyzer() {
    current_module = nullptr;
    current_symbol_table = nullptr;
}

SemanticAnalyzer::~SemanticAnalyzer() {

}

void SemanticAnalyzer::analyze_module(Module* module) {
    define_module_classes(module);
}

void SemanticAnalyzer::define_module_classes(Module* module) {
    current_module = module;
    current_symbol_table = module->get_symbol_table();

    for (auto klass : module->get_classes()) {
        define_class(klass);
    }

    current_module = nullptr;
    current_symbol_table = nullptr;
}

void SemanticAnalyzer::define_class(Class* klass) {
    std::string name = klass->get_name().get_value();

    auto symbols = current_symbol_table->resolve_local(name);

    if (symbols.size() == 0) {
        auto symbol = new Symbol(SYM_CLASS, klass);
        current_symbol_table->define(symbol);
        log_info("Defining class <white>'" + name + "'<normal>");
    } else {
        std::stringstream ss;
        Function* f;
        UserType* ut;

        switch (symbols[0]->get_kind()) {
        case SYM_FUNCTION:
            f = (Function*) symbols[0]->get_descriptor();

            break;

        case SYM_CLASS:
        case SYM_STRUCT:
        case SYM_ENUM:
        case SYM_UNION:
            ut = (UserType*) symbols[0]->get_descriptor();
            ss << "<white>'" + name + "'<normal> already defined\n";
            ss << "    other occurence here:\n";
            auto line = get_line_from_file("/home/hadley/Projects/haard/tests/semantic/t1.hd", ut->get_name().get_line());

            ss << "    ";
            ss << line;
            log_error(ss.str());
            break;
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
