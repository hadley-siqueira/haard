#ifndef HAARD_AST_WHILE_STATEMENT_H
#define HAARD_AST_WHILE_STATEMENT_H

#include "haard/ast/statements/statement.h"
#include "haard/ast/statements/statements.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class WhileStatement : public Statement {
    public:
        WhileStatement();
        ~WhileStatement();

    public:
        Statements* get_statements() const;
        void set_statements(Statements* statements);

        Expression* get_expression() const;
        void set_expression(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        Statements* statements;
        Expression* expression;
    };
}

#endif
