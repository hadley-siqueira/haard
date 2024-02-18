#ifndef HAARD_AST_RETURN_STATEMENT_H
#define HAARD_AST_RETURN_STATEMENT_H

#include "ast/statement.h"
#include "ast/compound_statement.h"
#include "ast/expression.h"

namespace haard {
    class ReturnStatement : public Statement {
    public:
        ReturnStatement();
        ~ReturnStatement();

    public:
        Expression* get_expression();
        void set_expression(Expression* expression);

    private:
        Expression* expression;
    };
}

#endif
