#ifndef HAARD_AST_SEQUENCE_H
#define HAARD_AST_SEQUENCE_H

#include <vector>

#include "haard/ast/expressions/expression.h"

namespace haard {
    class Sequence : public Expression {
    public:
        Sequence();
        ~Sequence();

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
