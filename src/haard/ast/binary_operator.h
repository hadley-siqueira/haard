#ifndef HAARD_AST_BINARY_OPERATOR_H
#define HAARD_AST_BINARY_OPERATOR_H

#include <haard/token/token.h>
#include <haard/ast/expression.h>

namespace haard {
    class BinaryOperator : public Expression {
    public:
        BinaryOperator();
        BinaryOperator(AstKind kind, const Token& token, Expression* left, Expression* right);
        ~BinaryOperator();

    public:
        void set_token(const Token& token);
        Token get_token();

        void set_left(Expression* expression);
        Expression* get_left();

        void set_right(Expression* expression);
        Expression* get_right();

    private:
        Token token;
        Expression* left;
        Expression* right;
    };
}

#endif
