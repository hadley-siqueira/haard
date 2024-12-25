#ifndef HAARD_AST_USER_TYPE_H
#define HAARD_AST_USER_TYPE_H

#include <vector>

#include "haard/ast/ast_node.h"
#include "haard/ast/function.h"
#include "haard/ast/variable.h"
#include "haard/ast/generics.h"
#include "haard/ast/types/type.h"

namespace haard {
    class UserType : public AstNode {
    public:
        UserType();
        ~UserType();

    public:
        void add_variable(Variable* var);
        void add_function(Function* function);

        Generics* get_generics() const;
        void set_generics(Generics* generics);

        std::vector<Variable*>& get_variables();
        void set_variables(const std::vector<Variable*>& variables);

        std::vector<Function *>& get_functions();
        void set_functions(const std::vector<Function*>& functions);

        Token get_name() const;
        void set_name(const Token& token);

        Type* get_base_type() const;
        void set_base_type(Type* base_type);

    private:
        Token name;
        Generics* generics;
        Type* base_type;
        std::vector<Variable*> variables;
        std::vector<Function*> functions;
    };
}

#endif
