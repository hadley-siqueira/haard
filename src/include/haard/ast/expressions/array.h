#ifndef HAARD_AST_ARRAY_H
#define HAARD_AST_ARRAY_H

#include <vector>

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Array : public Expression {
    public:
        Array();
        ~Array();

    public:
        void add_expression(Expression* expression);

        std::vector<Expression*>& get_expressions();

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        std::vector<Expression*> expressions;
    };
}

#endif
