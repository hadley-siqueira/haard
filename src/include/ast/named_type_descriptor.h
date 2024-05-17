#ifndef HAARD_NAMED_TYPE_DESCRIPTOR_H
#define HAARD_NAMED_TYPE_DESCRIPTOR_H

#include <vector>
#include "ast/declaration.h"
#include "ast/variable.h"
#include "ast/function.h"
#include "ast/type_list.h"
#include "scope/scope.h"

namespace haard {
    class NamedTypeDescriptor : public Declaration {
    public:
        NamedTypeDescriptor();
        virtual ~NamedTypeDescriptor();

    public:
        void add_function(Function* function);
        void add_variable(Variable* variable);

        int functions_count();
        int variables_count();

        Function* get_function(int idx);
        Variable* get_variable(int idx);

        TypeList* get_generics() const;
        void set_generics(TypeList* newGenerics);

        Type* get_super_type() const;
        void set_super_type(Type* newSuper_type);

        Type* get_self_type();
        void set_self_type(Type* type);

        virtual std::string get_qualified_name();
        virtual std::string get_cpp_namespace();

        Scope *get_scope() const;
        void set_scope(Scope *newScope);

    private:
        TypeList* generics;
        Type* super_type;
        Type* self_type;
        Scope* scope;
        std::vector<Function*> functions;
        std::vector<Variable*> variables;
    };
}

#endif
