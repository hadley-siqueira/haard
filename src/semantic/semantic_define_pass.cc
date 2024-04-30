#include <iostream>

#include "semantic/semantic_define_pass.h"

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
    enter_scope(module->get_scope());

    for (int i = 0; i < module->classes_count(); ++i) {
        define_class(module->get_class(i));
    }

    leave_scope();
}

void SemanticDefinePass::define_module_functions(Module* module) {
    enter_scope(module->get_scope());

    for (int i = 0; i < module->functions_count(); ++i) {
        define_function(module->get_function(i));
    }

    leave_scope();
}

void SemanticDefinePass::build_modules_functions(Modules* modules) {
    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module_functions(modules->get_module(i));
    }
}

void SemanticDefinePass::build_module_functions(Module* module) {
    enter_scope(module->get_scope());

    for (int i = 0; i < module->functions_count(); ++i) {
        build_function(module->get_function(i));
    }

    leave_scope();
}

void SemanticDefinePass::build_function(Function* function) {
    enter_scope(function->get_scope());

    build_statement(function->get_statements());

    leave_scope();
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

    if (sym == nullptr) {
        define(SYM_CLASS, name, klass);
        logger->info("defining class " + name + " (" + klass->get_qualified_name() + ")");
        return;
    } else {
        logger->error(name + " already defined");
    }
}

void SemanticDefinePass::define_function(Function* function) {
    std::string name = function->get_name().get_value();

    Symbol* sym = current_scope->resolve_in_module(name);

    if (sym == nullptr) {
        define(SYM_FUNCTION, name, function);
        logger->info("defining function " + name);
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

void SemanticDefinePass::build_expression(Expression* expr) {
    if (expr == nullptr) {
        logger->error("semantic: expr == null");
        return;
    }

    int kind = expr->get_kind();

    switch (kind) {
    case AST_ID:
        build_identifier((Identifier*) expr);
        break;

    case EXPR_ASSIGNMENT:
        build_assignment((Assignment*) expr);
        break;

    case EXPR_PLUS:
        build_plus((Plus*) expr);
        break;
    }
}

/* This method checks for simple identifiers. For instance a and my_id
 * not a.foo.bar or a[2] or foo(2, 3) */
void SemanticDefinePass::build_identifier(Identifier* expr) {
    std::string name = expr->get_name().get_value();

    Symbol* sym = resolve(name);

    if (sym == nullptr) {
        logger->error("'"+ name + "' not on scope");
    } else {
        expr->set_symbol(sym);
    }
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
}

Function* SemanticDefinePass::check_for_overloaded(Symbol* sym, Function* function) {
    Function* other = nullptr;

    for (int i = 0; i < sym->descriptors_count(); ++i) {
        other = (Function*) sym->get_descriptor(i);

        if (false /*function->equals(other)*/) {
            return other;
        }
    }

    return nullptr;
}
