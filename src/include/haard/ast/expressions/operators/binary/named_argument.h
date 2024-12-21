#ifndef HAARD_AST_NAMED_ARGUMENT_H
#define HAARD_AST_NAMED_ARGUMENT_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class NamedArgument : public Expression {
    public:
        NamedArgument();
        ~NamedArgument();

    public:
        Token get_name() const;
        void set_name(const Token& name);

        Expression* get_expression() const;
        void set_expression(Expression* expression);

    private:
        Token name;
        Expression* expression;
    };
}

#endif
