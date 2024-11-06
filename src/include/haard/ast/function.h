#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>

#include "haard/ast/ast.h"
#include "haard/ast/types/type.h"
#include "haard/ast/variable.h"
#include "haard/ast/generics.h"

namespace haard {
    class Function : public Ast {
    public:
        Function();
        ~Function();

        const Token& get_name() const;
        void set_name(const Token& newName);

        Type* get_return_type() const;
        void set_return_type(Type* type);

        const std::vector<Variable*>& get_parameters() const;
        void set_parameters(const std::vector<Variable* >& parameters);

        const Generics* get_generics() const;
        void set_generics(Generics* generics);

        virtual std::string to_json();
        virtual std::string to_str();

        Ast* get_statements() const;
        void set_statements(Ast* statements);

    private:
        Token name;
        Type* return_type;
        std::vector<Variable*> parameters;
        Generics* generics;
        Ast* statements;
    };
}

#endif
