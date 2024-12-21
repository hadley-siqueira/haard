#ifndef HAARD_AST_New_H
#define HAARD_AST_New_H

#include <vector>

#include "haard/ast/expressions/expression.h"

namespace haard {
    class New : public Expression {
    public:
        New();
        New(Token& token, Expression* expression=nullptr);
        ~New();

    public:
        void add_argument(Expression* expression);
        std::vector<Expression*>& get_arguments();

        Token get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        std::vector<Expression*> arguments;

    };
}

#endif
