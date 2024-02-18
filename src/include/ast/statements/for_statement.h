#ifndef HAARD_AST_FOR_STATEMENT_H
#define HAARD_AST_FOR_STATEMENT_H

#include "ast/statement.h"
#include "ast/compound_statement.h"
#include "ast/expression.h"
#include "ast/expression_list.h"

namespace haard {
    class ForStatement : public Statement {
    public:
        ForStatement();
        ~ForStatement();

    public:
        Expression* get_test() const;
        void set_test(Expression* newTest);

        ExpressionList* get_init() const;
        void set_init(ExpressionList* newInit);

        ExpressionList* get_update() const;
        void set_update(ExpressionList* newUpdate);

        Expression* get_range() const;
        void set_range(Expression* newRange);

        CompoundStatement *get_statements() const;
        void set_statements(CompoundStatement *newStatements);

        bool is_foreach();

    private:
        Expression* test;
        ExpressionList* init;
        ExpressionList* update;
        Expression* range;
        CompoundStatement* statements;
    };
}

#endif
