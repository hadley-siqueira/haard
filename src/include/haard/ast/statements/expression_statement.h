#ifndef HAARD_AST_EXPRESSION_STATEMENT_H
#define HAARD_AST_EXPRESSION_STATEMENT_H

#include "haard/ast/statements/statement.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class ExpressionStatement : public Statement {
    public:
        ExpressionStatement();
        ExpressionStatement(Expression* expression);
        ~ExpressionStatement();

    public:
        Expression* get_expression() const;
        void set_expression(Expression* expression);

    private:
        Expression* expression;
    };
}

#endif
