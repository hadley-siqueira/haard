#ifndef HAARD_AST_IF_STATEMENT_H
#define HAARD_AST_IF_STATEMENT_H

#include "haard/ast/statements/statement.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class IfStatement : public Statement {
    public:
        IfStatement();
        ~IfStatement();

    public:
        Statement* get_true_statements() const;
        void set_true_statements(Statement* statements);

        Statement* get_false_statements() const;
        void set_false_statements(Statement* statements);

        Expression* get_expression() const;
        void set_expression(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        Statement* true_statements;
        Statement* false_statements;
        Expression* expression;
    };
}

#endif
