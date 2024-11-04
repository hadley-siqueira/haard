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
            Ast* other_params = other->get_child(AST_PARAMETERS);
            Ast* params = f->get_child(AST_PARAMETERS);

            if (other_params->children_count() == params->children_count()) {
                bool same = true;

                for (int i = 0; i < other_params->children_count(); ++i) {
                    Ast* t1 = other_params->get_child(i)->get_child(AST_PARAMETER_TYPE)->get_child();
                    Ast* t2 = params->get_child(i)->get_child(AST_PARAMETER_TYPE)->get_child();

                    if (!equal_types(t1, t2, current_scope)) {
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
        Ast* params = f->get_child(AST_PARAMETERS);

        if (params) {
            int i;

            for (i = 0; i < params->children_count() - 1; ++i) {
                PrettyPrinter pp;
                pp.print(params->get_child(i)->get_child(AST_PARAMETER_TYPE)->get_child());
                msg << pp.get_output() << ", ";
            }

            PrettyPrinter pp;
            pp.print(params->get_child(i)->get_child(AST_PARAMETER_TYPE)->get_child());
            msg << pp.get_output();
        }

        msg << ")'";
        log_info(msg.str());
    }
}

bool SemanticAnalyzer::equal_types(Ast* t1, Ast* t2, Scope* scope) {
    if (t1->get_kind() != t2->get_kind()) {
        return false;
    }

    if (t1->get_kind() == AST_TYPE_POINTER || t2->get_kind() == AST_TYPE_REFERENCE) {
        return equal_types(t1->get_child(), t2->get_child(), scope);
    } else if (t1->get_kind() == AST_TYPE_NAMED) {
        return equal_named_types(t1, t2, scope);
    } else {
        return true;
    }

    return false;
}

bool SemanticAnalyzer::equal_named_types(Ast* t1, Ast* t2, Scope* scope) {
    std::string name1;
    std::string name2;
    std::string alias1;
    std::string alias2;

    alias1 = get_named_type_alias(t1);
    alias2 = get_named_type_alias(t2);
    name1 = get_named_type_name(t1);
    name2 = get_named_type_name(t2);
    bool same_name = false;

    if (alias1 == alias2) {
        if (name1 == name2) {
            same_name = true;
        }
    } else if (alias1 == "" && alias2 == "::" || alias1 == "::" && alias2 == "") {
        if (name1 == name2) {
            same_name = true;
        }
    }

    if (same_name) {
        Ast* g1 = get_named_type_generics(t1);
        Ast* g2 = get_named_type_generics(t2);

        if (g1 != nullptr && g2 != nullptr) {
            if (g1->children_count() != g2->children_count()) {
                same_name = false;
            } else {
                for (int i = 0; i < g1->children_count(); ++i) {
                    if (!equal_types(g1->get_child(i), g2->get_child(i), scope)) {
                        same_name = false;
                        break;
                    }
                }
            }

        } else if (g1 == nullptr && g2 != nullptr) {
            same_name = false;
        } else if (g1 != nullptr && g2 == nullptr) {
            same_name = false;
        }
    }

    return same_name;
}

std::string SemanticAnalyzer::get_named_type_name(Ast* type) {
    std::string name;
    type = type->get_child();

    /* A named type can come as:
     * - name (AST_ID)
     * - ::name (AST_SCOPE)
     * - alias::name (AST_SCOPE)
     * - alias::name<types> (AST_GENERIC_APPLICATION)
     *
     * In all cases, there is a name to be fetched, we then retrieving until
     * we get to the name */
    if (type->get_kind() == AST_GENERIC_APPLICATION) {
        type = type->get_child(); // extract the AST_SCOPE inside
    }

    if (type->get_kind() == AST_SCOPE) {
        type = type->get_child(); // extract the AST_ID representing the name
    }

    // when we reach here, type must be pointing to an AST_ID
    return type->get_value();
}

std::string SemanticAnalyzer::get_named_type_alias(Ast* type) {
    std::string alias;
    type = type->get_child();

    if (type->get_kind() == AST_GENERIC_APPLICATION) {
        type = type->get_child(); // extract the AST_SCOPE inside
    }

    if (type->get_kind() == AST_SCOPE) {
        if (type->children_count() == 1) {
            alias = "::";
        } else {
            alias = type->get_child(1)->get_value();
        }
    }

    return alias;
}

Ast* SemanticAnalyzer::get_named_type_generics(Ast* type) {
    type = type->get_child();

    if (type->get_kind() == AST_GENERIC_APPLICATION) {
        return type->get_child(1); // extract the AST_GENERICS
    }

    return nullptr;
}
