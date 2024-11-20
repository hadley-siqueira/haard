#ifndef HAARD_AST_UNARY_OPERATOR_H
#define HAARD_AST_UNARY_OPERATOR_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class UnaryOperator : public Expression {
    public:
        UnaryOperator();
        ~UnaryOperator();

    public:
        Expression* get_expression() const;
        void set_expression(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& newToken);

    private:
        Token token;
        Expression* expression;
    };
}

#endif
