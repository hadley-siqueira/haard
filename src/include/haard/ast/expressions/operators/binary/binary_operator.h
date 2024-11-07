#ifndef HAARD_AST_BINARY_OPERATOR_H
#define HAARD_AST_BINARY_OPERATOR_H

#include "haard/ast/expressions/expression.h"

namespace haard {
    class BinaryOperator : public Expression {
    public:
        BinaryOperator();
        ~BinaryOperator();

    public:

        Expression* get_left() const;
        void set_left(Expression* expression);

        Expression* get_right() const;
        void set_right(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& newToken);

    private:
        Token token;
        Expression* left;
        Expression* right;
    };
}

#endif
