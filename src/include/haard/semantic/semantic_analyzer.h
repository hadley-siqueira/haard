#ifndef HAARD_SEMANTIC_ANALYZER_H
#define HAARD_SEMANTIC_ANALYZER_H

#include "haard/ast/ast.h"
#include "haard/scope/scope.h"

namespace haard {
    class SemanticAnalyzer {
    public:
        SemanticAnalyzer();
        ~SemanticAnalyzer();

    public:
        void process_module(Ast* mod);

    public:
        void declare_module_user_types(Ast* mod);
        void declare_user_type(Ast* type);
        void declare_module_functions(Ast* mod);
        void declare_function(Ast* function);

        void analyze_module_functions(Ast* mod);
        void analyze_function(Ast* function);

        void analyze_statement(Ast* stmt);
        void analyze_compound_statement(Ast* stmts);
        void analyze_while_statement(Ast* stmt);

        void analyze_expression(Ast* expr);
        void analyze_assignment(Ast* expr);
        void analyze_identifier(Ast* expr);

        bool equal_types(Ast* t1, Ast* t2);
        bool compatible_types(Ast* t1, Ast* t2);

        Ast* get_expression_type(Ast* expr, Scope* scope);
        Ast* get_identifier_type(Ast* expr, Scope* scope);

    private:
        Scope* current_scope;
    };
}

#endif
