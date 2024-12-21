#ifndef HAARD_AST_CALL_H
#define HAARD_AST_CALL_H

#include <vector>

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Call : public Expression {
    public:
        Call();
        ~Call();

    public:
        void add_argument(Expression* expression);
        std::vector<Expression*>& get_arguments();

        Expression* get_expression() const;
        void set_expression(Expression* expression);

        Token get_token() const;
        void set_token(const Token &token);

    private:
        Token token;
        Expression* expression;
        std::vector<Expression*> arguments;
    };
}

#endif
