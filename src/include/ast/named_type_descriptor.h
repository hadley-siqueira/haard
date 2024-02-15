#ifndef HAARD_NAMED_TYPE_DESCRIPTOR_H
#define HAARD_NAMED_TYPE_DESCRIPTOR_H

#include <vector>
#include "ast/declaration.h"
#include "ast/variable.h"
#include "ast/function.h"
#include "ast/type_list.h"

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

        Type *get_super_type() const;
        void set_super_type(Type *newSuper_type);

    private:
        TypeList* generics;
        Type* super_type;
        std::vector<Function*> functions;
        std::vector<Variable*> variables;
    };
}

#endif
