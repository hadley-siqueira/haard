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

    analyze_module_functions(mod);
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

void SemanticAnalyzer::declare_function(Ast* function) {
    bool not_declared = true;
    std::stringstream msg;
    std::string name = function->get_value();

    auto symbols = current_scope->resolve_local(name);

    if (symbols.size() > 0) {
        for (auto sym : symbols) {
            Ast* other = sym->get_ast();
            auto other_params = other->get_children(AST_VARIABLE);
            auto params = function->get_children(AST_VARIABLE);

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
        current_scope->define(SYM_FUNCTION, name, function);

        msg << "Defining function '" + name + "(";
        auto params = function->get_children(AST_VARIABLE);

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

void SemanticAnalyzer::analyze_module_functions(Ast* mod) {
    for (auto f : mod->get_children(AST_FUNCTION)) {
        analyze_function(f);
    }
}

void SemanticAnalyzer::analyze_function(Ast* function) {
    analyze_compound_statement(function->get_child(AST_STATEMENTS));
}

void SemanticAnalyzer::analyze_statement(Ast* stmt) {
    auto kind = stmt->get_kind();

    switch (kind) {
    case AST_STATEMENTS:
        analyze_compound_statement(stmt);
        break;

    case AST_WHILE:
        analyze_while_statement(stmt);
        break;

    case AST_EXPRESSION:
        analyze_expression(stmt->get_child());
        break;

    default:
        break;
    }
}

void SemanticAnalyzer::analyze_compound_statement(Ast* stmts) {
    for (auto stmt : stmts->get_children()) {
        analyze_statement(stmt);
    }
}

void SemanticAnalyzer::analyze_while_statement(Ast* stmt) {

}

void SemanticAnalyzer::analyze_expression(Ast* expr) {
    auto kind = expr->get_kind();

    switch (kind) {
    case AST_ASSIGNMENT:
        analyze_assignment(expr);
        break;

    case AST_IDENTIFIER:
        analyze_identifier(expr);
        break;

    default:
        break;
    }
}

void SemanticAnalyzer::analyze_assignment(Ast* expr) {
    Ast* left = expr->get_child(0);
    Ast* right = expr->get_child(1);

    analyze_expression(right);
    Ast* t2 = get_expression_type(right, current_scope);

    if (left->get_kind() == AST_IDENTIFIER) {
        std::string name = left->get_value();
        auto symbols = current_scope->resolve(name);

        if (symbols.size() == 0) {
            current_scope->define(SYM_VARIABLE, name, expr, t2->clone());
            PrettyPrinter pp;
            pp.print(t2);
            log_info("Declaring variable " + name + " : " + pp.get_output());
        }
    }

    Ast* t1 = get_expression_type(left, current_scope);

    if (!compatible_types(t1, t2)) {
        log_error("wrong assignment: incompatible types");
    }

    delete t1;
    delete t2;
}

void SemanticAnalyzer::analyze_identifier(Ast* expr) {
    std::string name = expr->get_value();

    auto symbols = current_scope->resolve(name);

    if (symbols.size() == 0) {
        log_error(name + " not in scope");
    }
}

bool SemanticAnalyzer::equal_types(Ast* t1, Ast* t2) {
    PrettyPrinter p1;
    PrettyPrinter p2;

    p1.print(t1);
    p2.print(t2);

    return p1.get_output() == p2.get_output();
}

bool SemanticAnalyzer::compatible_types(Ast* t1, Ast* t2) {
    return equal_types(t1, t2);
}

Ast* SemanticAnalyzer::get_expression_type(Ast* expr, Scope* scope) {
    auto kind = expr->get_kind();

    switch (kind) {
    case AST_IDENTIFIER:
        return get_identifier_type(expr, scope);

    case AST_LITERAL_INTEGER:
        return new Ast(AST_TYPE_I32);

    case AST_LITERAL_BOOLEAN:
        return new Ast(AST_TYPE_BOOL);

    case AST_LITERAL_FLOAT:
        return new Ast(AST_TYPE_F32);

    case AST_LITERAL_DOUBLE:
        return new Ast(AST_TYPE_F64);

    default:
        break;
    }

    return nullptr;
}

Ast* SemanticAnalyzer::get_identifier_type(Ast* expr, Scope* scope) {
    std::string name;
    name = expr->get_value();

    auto symbols = scope->resolve(name);

    if (symbols.size() == 1) {
        return symbols[0]->get_type()->clone();
    }

    return nullptr;
}
