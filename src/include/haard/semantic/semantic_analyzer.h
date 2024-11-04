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
        void declare_function(Ast* f);

        bool equal_types(Ast* t1, Ast* t2, Scope* scope);
        bool equal_named_types(Ast* t1, Ast* t2, Scope* scope);

        std::string get_named_type_name(Ast* type);
        std::string get_named_type_alias(Ast* type);
        Ast* get_named_type_generics(Ast* type);

    private:
        Scope* current_scope;
    };
}

#endif
