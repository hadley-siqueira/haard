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
    define_module_functions(module);
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

void SemanticAnalyzer::define_module_functions(Module* module) {
    current_module = module;
    current_symbol_table = module->get_symbol_table();

    for (auto f : module->get_functions()) {
        define_function(f);
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
            auto line = get_line_from_file(ut->get_path(), ut->get_name().get_line());

            ss << "    ";
            ss << line;
            log_error(ss.str());
            break;
        }
    }
}

void SemanticAnalyzer::define_function(Function* function) {
    std::string name = function->get_name().get_value();

    auto symbols = current_symbol_table->resolve_local(name);

    if (symbols.size() == 0) {
        auto symbol = new Symbol(SYM_FUNCTION, function);
        current_symbol_table->define(symbol);
        log_info("Defining function <white>'" + name + "'<normal>");
    } else {
        for (auto symbol : symbols) {
            auto kind = symbol->get_kind();
            std::stringstream ss;
            Function* f;
            UserType* ut;
            std::string line;

            switch (kind) {
            case SYM_FUNCTION:
                f = (Function*) symbols[0]->get_descriptor();
                ss << "<white>'" + name + "'<normal> already defined\n";
                ss << "    other occurence here:\n"; // TODO: fix path with implementation, refactor redefinition
                line = get_line_from_file(f->get_path(), f->get_name().get_line());

                ss << "    ";
                ss << line;
                log_error(ss.str());

                break;

            case SYM_CLASS:
            case SYM_STRUCT:
            case SYM_ENUM:
            case SYM_UNION:
                ut = (UserType*) symbols[0]->get_descriptor();
                ss << "<white>'" + name + "'<normal> already defined\n";
                ss << "    other occurence here:\n";
                line = get_line_from_file(ut->get_path(), ut->get_name().get_line());

                ss << "    ";
                ss << line;
                log_error(ss.str());
                break;
            }
        }
    }
}
