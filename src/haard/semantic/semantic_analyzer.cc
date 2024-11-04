#include <iostream>
#include <sstream>

#include "haard/semantic/semantic_analyzer.h"
#include "haard/pretty_printer/pretty_printer.h"
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
    std::stringstream msg;
    bool not_declared = true;

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

    if (symbols.size() > 0) {
        for (auto sym : symbols) {
            Ast* other = sym->get_ast();

            Ast* g1 = type->get_child(AST_GENERICS);
            Ast* g2 = other->get_child(AST_GENERICS);

            if (g1 && g2) {
                if (g1->children_count() == g2->children_count()) {
                    log_error("'" + name + "' already defined");
                    not_declared = false;
                }
            } else if (g1 == nullptr && g2 == nullptr) {
                log_error("User type '" + name + "' already defined");
                not_declared = false;
            }
        }
    }

    if (not_declared) {
        current_scope->define(kind, name, type);

        Ast* g1 = type->get_child(AST_GENERICS);
        msg << "Defining " + kind_as_str + " '" + name;

        if (g1) {
            PrettyPrinter p;
            p.print(g1);
            msg << p.get_output();
        }

        msg << "'";
        log_info(msg.str());
    }
}

void SemanticAnalyzer::declare_module_functions(Ast* mod) {
    auto functions = mod->get_children(AST_FUNCTION);

    for (auto f : functions) {
        declare_function(f);
    }
}

void SemanticAnalyzer::declare_function(Ast* f) {
    bool not_declared = true;
    std::stringstream msg;
    std::string name = f->get_value();

    auto symbols = current_scope->resolve_local(name);

    if (symbols.size() > 0) {
        for (auto sym : symbols) {
            Ast* other = sym->get_ast();
            auto other_params = other->get_children(AST_VARIABLE);
            auto params = f->get_children(AST_VARIABLE);

            if (other_params.size() == params.size()) {
                bool same = true;

                for (int i = 0; i < other_params.size(); ++i) {
                    Ast* t1 = other_params[i]->get_child(AST_TYPE)->get_child();
                    Ast* t2 = params[i]->get_child(AST_TYPE)->get_child();

                    if (!equal_types(t1, t2)) {
                        same = false;
                        break;
                    }
                }

                if (same) {
                    not_declared = false;
                    log_error("Function " + name + " already declared");
                }
            }
        }
    }

    if (not_declared) {
        current_scope->define(SYM_FUNCTION, name, f);

        msg << "Defining function '" + name + "(";
        auto params = f->get_children(AST_VARIABLE);

        if (params.size() > 0) {
            int i;

            for (i = 0; i < params.size() - 1; ++i) {
                PrettyPrinter pp;
                pp.print(params[i]->get_child(AST_TYPE)->get_child());
                msg << pp.get_output() << ", ";
            }

            PrettyPrinter pp;
            pp.print(params[i]->get_child(AST_TYPE)->get_child());
            msg << pp.get_output();
        }

        msg << ")'";
        log_info(msg.str());
    }
}

bool SemanticAnalyzer::equal_types(Ast* t1, Ast* t2) {
    PrettyPrinter p1;
    PrettyPrinter p2;

    p1.print(t1);
    p2.print(t2);

    return p1.get_output() == p2.get_output();
}
