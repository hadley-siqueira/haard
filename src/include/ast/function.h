#ifndef HAARD_FUNCTION_H
#define HAARD_FUNCTION_H

#include <vector>
#include "ast/declaration.h"
#include "ast/variable.h"
#include "ast/type.h"
#include "ast/type_list.h"
#include "ast/compound_statement.h"


namespace haard {
    class Scope;

    class Function : public Declaration {
    public:
        Function();
        ~Function();

        void add_parameter(Variable* param);
        int parameters_count();
        Variable* get_parameter(int idx);

        Type* get_return_type() const;
        void set_return_type(Type* newReturn_type);

        TypeList* get_generics() const;
        void set_generics(TypeList* newGenerics);

        CompoundStatement* get_statements() const;
        void set_statements(CompoundStatement *newStatements);

        Scope* get_scope() const;
        void set_scope(Scope* newScope);

    private:
        Type* return_type;
        TypeList* generics;
        Scope* scope;
        CompoundStatement* statements;
        std::vector<Variable*> parameters;
    };
}

#endif
