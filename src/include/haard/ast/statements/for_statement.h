#ifndef HAARD_AST_FOR_STATEMENT_H
#define HAARD_AST_FOR_STATEMENT_H

#include <vector>

#include "haard/ast/statements/statement.h"
#include "haard/ast/statements/statements.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class ForStatement : public Statement {
    public:
        ForStatement();
        ~ForStatement();

    public:
        Statements* get_statements() const;
        void set_statements(Statements* statements);

        Expression* get_test() const;
        void set_test(Expression* expression);

        const Token& get_token() const;
        void set_token(const Token& token);

        std::vector<Expression*>& get_initialization();
        void set_initialization(const std::vector<Expression*>& initialization);

        std::vector<Expression *>& get_update();
        void set_update(const std::vector<Expression *>& update);

    private:
        Token token;
        Statements* statements;
        Expression* test;
        std::vector<Expression*> initialization;
        std::vector<Expression*> update;
    };
}

#endif
