#ifndef HAARD_FUNCTION_H
#define HAARD_FUNCTION_H

#include <vector>
#include "ast/declaration.h"
#include "ast/variable.h"
#include "ast/types/type.h"
#include "ast/type_list.h"
#include "ast/compound_statement.h"

namespace haard {
    class Scope;
    class NamedTypeDescriptor;

    class Function : public Declaration {
    public:
        Function();
        ~Function();

        void add_parameter(Variable* param);
        int parameters_count();
        Variable* get_parameter(int idx);

        void add_variable(Variable* var);
        int variables_count();
        Variable* get_variable(int idx);

        Type* get_return_type() const;
        void set_return_type(Type* newReturn_type);

        TypeList* get_generics() const;
        void set_generics(TypeList* newGenerics);

        CompoundStatement* get_statements() const;
        void set_statements(CompoundStatement *newStatements);

        Scope* get_scope() const;
        void set_scope(Scope* newScope);

        bool is_method();
        bool is_constructor();
        bool is_destructor();

        NamedTypeDescriptor* get_named_type_descriptor() const;
        void set_named_type_descriptor(NamedTypeDescriptor* newNamed_type_descriptor);
        std::string get_cpp_namespace();

    private:
        Type* return_type;
        TypeList* generics;
        Scope* scope;
        CompoundStatement* statements;
        NamedTypeDescriptor* named_type_descriptor;
        std::vector<Variable*> parameters;
        std::vector<Variable*> variables;
    };
}

#endif
