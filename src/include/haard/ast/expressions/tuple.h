#ifndef HAARD_AST_TUPLE_H
#define HAARD_AST_TUPLE_H

#include <vector>

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Tuple : public Expression {
    public:
        Tuple();
        ~Tuple();

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
