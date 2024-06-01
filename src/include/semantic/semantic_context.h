#ifndef HAARD_SEMANTIC_CONTEXT_H
#define HAARD_SEMANTIC_CONTEXT_H

#include <stack>

#include "ast/module.h"
#include "ast/class.h"
#include "ast/enum.h"
#include "ast/union.h"
#include "ast/struct.h"

namespace haard {
    class SemanticContext {
    public:
        SemanticContext();

    public:
        Module* get_module() const;
        void set_module(Module* newModule);

        Class* get_class() const;
        void set_class(Class* newKlass);

        Union* get_union() const;
        void set_union(Union* newUnion);

        Enum* get_enum() const;
        void set_enum(Enum* newEnum);

        Struct* get_struct() const;
        void set_struct(Struct* newStruct);

        Function* get_function() const;
        void set_function(Function* newFunction);

        Scope* get_scope() const;
        void set_scope(Scope *newScope);

        void push_scope(Scope* scope);

    private:
        Module* module;
        Class* klass;
        Union* union_;
        Enum* enum_;
        Struct* struct_;
        Function* function;
        Scope* scope;
        std::stack<Scope*> scope_stack;
    };
}

#endif
