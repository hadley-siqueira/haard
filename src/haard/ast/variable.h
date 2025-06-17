#ifndef HAARD_AST_VARIABLE_H
#define HAARD_AST_VARIABLE_H

#include <haard/ast/ast.h>
#include <haard/token/token.h>
#include <haard/ast/type.h>
#include <haard/ast/expression.h>

namespace haard {
    class Variable : public Ast {
    public:
        Variable();
        ~Variable();

    public:
        void set_name(const Token& token);
        Token& get_name();

        void set_const(bool value);

        void set_type(Type* type);
        Type* get_type();

        void set_expression(Expression* expression);
        Expression* get_expression();

        bool is_const();

    private:
        bool const_flag;
        Token name;
        Type* type;
        Expression* expression;
    };
}

#endif
