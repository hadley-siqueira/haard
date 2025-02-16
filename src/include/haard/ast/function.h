#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>

#include "haard/ast/ast_node.h"
#include "haard/ast/types/type.h"
#include "haard/ast/variable.h"
#include "haard/ast/generics.h"
#include "haard/ast/statements/statements.h"

namespace haard {
    class Function : public AstNode {
    public:
        Function();
        ~Function();

        const Token& get_name() const;
        void set_name(const Token& name);

        Type* get_return_type() const;
        void set_return_type(Type* type);

        const std::vector<Variable*>& get_parameters() const;
        void set_parameters(const std::vector<Variable* >& parameters);

        Generics* get_generics() const;
        void set_generics(Generics* generics);

        Statements* get_statements() const;
        void set_statements(Statements* statements);

        void add_parameter(Variable* parameter);

    private:
        Token name;
        Type* return_type;
        std::vector<Variable*> parameters;
        Generics* generics;
        Statements* statements;
    };
}

#endif
