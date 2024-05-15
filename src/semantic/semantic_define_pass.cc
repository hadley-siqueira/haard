#include <iostream>
#include <sstream>

#include "semantic/semantic_define_pass.h"
#include "ast/types/named_type.h"

using namespace haard;

void SemanticDefinePass::build_modules(Modules* modules) {
    define_modules_classes(modules);
    define_modules_function(modules);

    build_modules_functions(modules);
}

void SemanticDefinePass::define_modules_classes(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        define_module_classes(modules->get_module(i));
    }
}

void SemanticDefinePass::define_modules_function(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        define_module_functions(modules->get_module(i));
    }
}

void SemanticDefinePass::define_module_classes(Module* module) {
    current_module = module;
    enter_scope(module->get_scope());

    for (int i = 0; i < module->classes_count(); ++i) {
        define_class(module->get_class(i));
    }

    leave_scope();
    current_module = nullptr;
}

void SemanticDefinePass::define_module_functions(Module* module) {
    current_module = module;
    enter_scope(module->get_scope());

    for (int i = 0; i < module->functions_count(); ++i) {
        define_module_function(module->get_function(i));
    }

    leave_scope();
    current_module = nullptr;
}

void SemanticDefinePass::define_module_function(Function* function) {
    std::string fname = function->get_name().get_value();
    Symbol* sym = current_scope->resolve_local(fname);

    if (sym == nullptr) {
        current_scope->define(SYM_FUNCTION, fname, function);
        logger->info("defining function " + fname);
        return;
    }

    if (sym->is_function()) {
        Function* other = check_for_overloaded(sym, function);

        if (other != nullptr) {
            logger->error("Can't define function '" + fname + "'. Function already defined with same signature");
        } else {
            current_scope->define(SYM_FUNCTION, fname, function);
            logger->info("defining function " + fname);
        }
    } else {
        logger->error("Can't define function " + fname + ". Already defined");
    }
}

void SemanticDefinePass::build_modules_functions(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module_functions(modules->get_module(i));
    }
}

void SemanticDefinePass::build_module_functions(Module* module) {
    current_module = module;
    enter_scope(module->get_scope());

    for (int i = 0; i < module->functions_count(); ++i) {
        build_function(module->get_function(i));
    }

    leave_scope();
    current_module = nullptr;
}

void SemanticDefinePass::build_function(Function* function) {
    current_function = function;
    enter_scope(function->get_scope());

    build_statement(function->get_statements());

    leave_scope();
    current_function = nullptr;
}

void SemanticDefinePass::build_statement(Statement* stmt) {
    ExpressionStatement* expr = (ExpressionStatement*) stmt;

    if (stmt == nullptr) {
        return;
    }

    int kind = stmt->get_kind();

    switch (kind) {
    case STMT_COMPOUND:
        build_compound_statement((CompoundStatement*) stmt);
        break;

    case STMT_EXPRESSION:
        build_expression(expr->get_expression());
        break;
    }
}

void SemanticDefinePass::build_compound_statement(CompoundStatement* stmt) {
    enter_scope(stmt->get_scope());

    for (int i = 0; i < stmt->statements_count(); ++i) {
        build_statement(stmt->get_statement(i));
    }

    leave_scope();
}

void SemanticDefinePass::define_class(Class* klass) {
    std::string name = klass->get_name().get_value();

    Symbol* sym = current_scope->resolve_in_module(name);

    if (sym != nullptr) {
        logger->error(name + " already defined");
        return;
    }

    current_scope->define(SYM_CLASS, name, klass);
    logger->info("defining class " + name + " (" + klass->get_qualified_name() + ")");

    Identifier* id = new Identifier();
    id->set_name(klass->get_name());
    NamedType* named = new NamedType(id);
    build_type(named);
    klass->set_self_type(named);
}

void SemanticDefinePass::define_function(Function* function) {
    std::string name = function->get_name().get_value();

    Symbol* sym = current_scope->resolve_in_module(name);

    if (sym == nullptr) {
        define(SYM_FUNCTION, name, function);
        logger->info("defining function " + name);
        define_function_parameters(function);
        return;
    }

    if (sym->is_function()) {
        Function* other = check_for_overloaded(sym, function);

        if (other != nullptr) {
            logger->error("Can't define function. Function already defined with same signature");
        } else {
            define(SYM_FUNCTION, name, function);
            logger->info("defining function " + name);
        }
    } else {
        logger->error("Can't define function " + name + ". Already defined");
    }
}

void SemanticDefinePass::define_function_parameters(Function* function) {
    Scope* scope = function->get_scope();
    Symbol* sym;

    for (int i = 0; i < function->parameters_count(); ++i) {
        Variable* param = function->get_parameter(i);
        std::string param_name = param->get_name().get_value();

        sym = scope->resolve_local(param_name);

        if (sym == nullptr) {
            scope->define(SYM_PARAMETER, param_name, param);
        } else {
            logger->error("parameter " + param_name + " already defined");
        }
    }
}

void SemanticDefinePass::build_expression(Expression* expr) {
    if (expr == nullptr) {
        logger->error("semantic: expr == null");
        return;
    }

    int kind = expr->get_kind();

    switch (kind) {
    case EXPR_ASSIGNMENT:
        build_assignment((Assignment*) expr);
        break;

    case EXPR_CALL:
        build_call((Call*) expr);
        break;

    case EXPR_PLUS:
        build_plus((Plus*) expr);
        break;

    case AST_ID:
        build_identifier((Identifier*) expr);
        break;

    case EXPR_LITERAL_INTEGER:
        build_integer_literal((IntegerLiteral*) expr);
        break;
    }
}

void SemanticDefinePass::build_call(Call* expr) {
    int kind = expr->get_object()->get_kind();

    build_call_arguments(expr);

    switch (kind) {
    case AST_ID:
        build_simple_call(expr);
        break;
    }
}

void SemanticDefinePass::build_call_arguments(Call* expr) {
    build_expression_list(expr->get_arguments());
}

void SemanticDefinePass::build_simple_call(Call* expr) {
    Identifier* id = (Identifier*) expr->get_object();

    std::string name = id->get_name().get_value();
    Symbol* sym = current_scope->resolve(name);

    if (sym == nullptr) {
        logger->error("Callable not in scope: " + name);
        return;
    }

    SymbolKind kind = sym->get_kind();

    if (kind == SYM_FUNCTION) {
        bool found = false;

        for (int i = 0; i < sym->descriptors_count(); ++i) {
            Function* f = (Function*) sym->get_descriptor(i);

            if (f->parameters_count() == expr->get_arguments()->expressions_count()) {
                found = true;

                for (int j = 0; j < f->parameters_count(); ++j) {
                    Type* t1 = f->get_parameter(j)->get_type();
                    Type* t2 = expr->get_arguments()->get_expression(j)->get_type();

                    if (!t1->equals(t2)) {
                        found = false;
                    }
                }

                if (found) {
                    id->set_symbol(sym);
                    id->set_overload_index(i);
                    break;
                }
            }
        }

        if (!found) {
            logger->error("no matching argument types");
        }
    } else {
        logger->error("Trying to call " + name + " but it's not a function");
    }
}

/* This method checks for simple identifiers. For instance a and my_id
 * not a.foo.bar or a[2] or foo(2, 3) */
void SemanticDefinePass::build_identifier(Identifier* expr) {
    Symbol* sym;
    std::string name = expr->get_name().get_value();

    if (expr->has_alias()) {
        Import* imp = current_module->get_import_with_alias(expr->get_alias().get_value());
        sym = imp->get_module()->get_scope()->resolve_in_module(name);
    } else {
        sym = resolve(name);
    }

    if (sym == nullptr) {
        logger->error("'"+ name + "' not on scope");
    } else {
        expr->set_symbol(sym);
        expr->set_type(sym->get_type());
    }

    build_type(expr->get_type());
}

void SemanticDefinePass::build_integer_literal(IntegerLiteral* expr) {
    expr->set_type(new Type(TYPE_INT));
}

void SemanticDefinePass::build_assignment(Assignment* expr) {
    Expression* left = expr->get_left();
    Expression* right = expr->get_right();

    build_expression(right);

    if (is_new_variable_assignment(expr)) {
        create_local_variable_for_assignment(expr);
    }

    build_expression(left);
    expr->set_type(left->get_type());
}

void SemanticDefinePass::build_plus(Plus* expr) {
    Expression* left = expr->get_left();
    Expression* right = expr->get_right();

    build_expression(left);
    build_expression(right);
}

void SemanticDefinePass::build_type(Type* type) {

    if (type == nullptr) {
        return;
    }

    int kind = type->get_kind();

    switch (kind) {
    case TYPE_POINTER:
        build_pointer_type((PointerType*) type);
        break;

    case TYPE_REFERENCE:
        build_reference_type((ReferenceType*) type);
        break;

    case TYPE_LIST:
        build_list_type((ListType*) type);
        break;

    case TYPE_ARRAY:
        build_array_type((ArrayType*) type);
        break;

    case TYPE_TUPLE:
        build_tuple_type((TupleType*) type);
        break;

    case TYPE_FUNCTION:
        build_function_type((FunctionType*) type);
        break;

    case TYPE_NAMED:
        build_named_type((NamedType*) type);
        break;
    }
}

void SemanticDefinePass::build_pointer_type(PointerType* type) {
    build_type(type->get_subtype());
}

void SemanticDefinePass::build_reference_type(ReferenceType* type) {
    build_type(type->get_subtype());
}

void SemanticDefinePass::build_list_type(ListType* type) {
    build_type(type->get_subtype());
}

void SemanticDefinePass::build_array_type(ArrayType* type) {
    build_expression(type->get_expression());
    build_type(type->get_subtype());
}

void SemanticDefinePass::build_tuple_type(TupleType* type) {
    build_type_list(type->get_types());
}

void SemanticDefinePass::build_function_type(FunctionType* type) {
    build_type_list(type->get_param_types());
    build_type(type->get_return_type());
}

void SemanticDefinePass::build_named_type(NamedType* type) {
    std::string name = type->get_identifier()->get_name().get_value();

    Symbol* sym = resolve(name);

    if (sym == nullptr) {
        logger->error("Type " + name + " not in scope");
        return;
    }

    type->get_identifier()->set_symbol(sym);
}

void SemanticDefinePass::build_type_list(TypeList* types) {
    for (int i = 0; i < types->types_count(); ++i) {
        build_type(types->get_type(i));
    }
}

void SemanticDefinePass::build_expression_list(ExpressionList* exprs) {
    for (int i = 0; i < exprs->expressions_count(); ++i) {
        build_expression(exprs->get_expression(i));
    }
}

bool SemanticDefinePass::is_new_variable_assignment(Assignment* expr) {
    Expression* left = expr->get_left();

    if (left->get_kind() == AST_ID) {
        Identifier* id = (Identifier*) left;
        std::string name = id->get_name().get_value();
        Symbol* sym = resolve(name);

        return sym == nullptr;
    }

    return false;
}

void SemanticDefinePass::create_local_variable_for_assignment(Assignment* expr) {
    Variable* var = new Variable();
    Expression* left = expr->get_left();
    Identifier* id = (Identifier*) left;
    Token name = id->get_name();

    var->set_name(name);
    var->set_type(expr->get_right()->get_type());
    define(SYM_LOCAL_VARIABLE, name.get_value(), var);

    if (current_function) {
        current_function->add_variable(var);
    }
}

Function* SemanticDefinePass::check_for_overloaded(Symbol* sym, Function* function) {
    Function* other = nullptr;
    Type* t1;
    Type* t2;
    bool flag = true;
    int n1_params = function->parameters_count();

    for (int i = 0; i < sym->descriptors_count(); ++i) {
        other = (Function*) sym->get_descriptor(i);
        int n2_params = other->parameters_count();

        if (n1_params != n2_params) {
            continue;
        }

        flag = true;

        for (int j = 0; j < n1_params; ++j) {
            t1 = function->get_parameter(j)->get_type();
            t2 = other->get_parameter(j)->get_type();

            if (!t1->equals(t2)) {
                flag = false;
                break;
            }
        }

        if (flag) {
            return other;
        }
    }

    return nullptr;
}
