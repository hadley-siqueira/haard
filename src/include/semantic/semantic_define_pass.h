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
        void define_module_function(Function* function);

        void define_class(Class* klass);
        void define_function(Function* function);
        void define_function_parameters(Function* function);

        void build_modules_functions(Modules* modules);
        void build_module_functions(Module* module);

        void build_function(Function* function);

        void build_statement(Statement* stmt);
        void build_compound_statement(CompoundStatement* stmt);

        void build_expression(Expression* expr);
        void build_call(Call* expr);
        void build_call_arguments(Call* expr);
        void build_simple_call(Call* expr);

        void build_identifier(Identifier* expr);
        void build_integer_literal(IntegerLiteral* expr);

        void build_assignment(Assignment* expr);

        void build_plus(Plus* expr);

        void build_type(Type* type);
        void build_pointer_type(PointerType* type);
        void build_reference_type(ReferenceType* type);
        void build_list_type(ListType* type);
        void build_array_type(ArrayType* type);
        void build_tuple_type(TupleType* type);
        void build_function_type(FunctionType* type);
        void build_named_type(NamedType* type);
        void build_type_list(TypeList* types);

        void build_expression_list(ExpressionList* exprs);

    private:
        bool is_new_variable_assignment(Assignment* expr);
        void create_local_variable_for_assignment(Assignment* expr);

    private:
        Module* current_module;
        Function* current_function;
        Scope* current_scope;
        Function* check_for_overloaded(Symbol* sym, Function* function);
    };
}

#endif
