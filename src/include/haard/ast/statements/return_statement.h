#ifndef HAARD_AST_RETURN_H
#define HAARD_AST_RETURN_H

#include "haard/ast/statements/statement.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class ReturnStatement : public Statement {
    public:
        ReturnStatement(Token& token);
        ReturnStatement(Expression* expression=nullptr);
        ~ReturnStatement();

    public:
        Expression* get_expression() const;
        void set_expression(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        Expression* expression;
    };
}

#endif
