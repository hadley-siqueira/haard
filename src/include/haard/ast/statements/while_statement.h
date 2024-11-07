#ifndef HAARD_AST_WHILE_STATEMENT_H
#define HAARD_AST_WHILE_STATEMENT_H

#include "haard/ast/statements/statement.h"
#include "haard/ast/statements/compound_statement.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class WhileStatement : public Statement {
    public:
        WhileStatement();
        ~WhileStatement();

    public:
        CompoundStatement* get_statements() const;
        void set_statements(CompoundStatement* statements);

        Expression* get_expression() const;
        void set_expression(Expression* expression);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        CompoundStatement* statements;
        Expression* expression;
    };
}

#endif
