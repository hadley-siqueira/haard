#ifndef HAARD_SEMANTIC_FIRST_PASS_H
#define HAARD_SEMANTIC_FIRST_PASS_H

#include "semantic/semantic_pass.h"
#include "ast/ast.h"

namespace haard {
    class SemanticDefinePass : public SemanticPass {
    public:
        void build_modules(Modules* modules);
        void define_modules_classes(Modules* modules);
        void define_modules_function(Modules* modules);

        void define_module_classes(Module* module);
        void define_module_functions(Module* module);

        void define_class(Class* klass);
        void define_function(Function* function);

        void build_modules_functions(Modules* modules);
        void build_module_functions(Module* module);

        void build_function(Function* function);

        void build_statement(Statement* stmt);
        void build_compound_statement(CompoundStatement* stmt);

        void build_expression(Expression* expr);
        void build_identifier(Identifier* expr);

        void build_assignment(Assignment* expr);

        void build_plus(Plus* expr);

    private:
        bool is_new_variable_assignment(Assignment* expr);
        void create_local_variable_for_assignment(Assignment* expr);

    private:
        Function* check_for_overloaded(Symbol* sym, Function* function);
    };
}

#endif