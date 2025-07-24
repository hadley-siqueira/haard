#ifndef HAARD_AST_UNARY_OPERATOR_H
#define HAARD_AST_UNARY_OPERATOR_H

#include <haard/ast/expression.h>
#include <haard/token/token.h>

namespace haard {
    class UnaryOperator : public Expression {
    public:
        UnaryOperator();
        UnaryOperator(AstKind kind, Expression* expression, const Token& token);
        ~UnaryOperator();

    public:
        void set_token(const Token& token);
        Token get_token();

        void set_expression(Expression* expression);
        Expression* get_expression();

    private:
        Token token;
        Expression* expression;
    };
}

#endif
