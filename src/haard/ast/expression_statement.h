#ifndef HAARD_AST_EXPRESSION_STATEMENT_H
#define HAARD_AST_EXPRESSION_STATEMENT_H

#include <haard/ast/statement.h>
#include <haard/ast/expression.h>

namespace haard {
    class ExpressionStatement : public Statement {
    public:
        ExpressionStatement(Expression* expression=nullptr);
        ~ExpressionStatement();

    public:
        void set_expression(Expression* expression);
        Expression* get_expression();

        void set_semicolon(bool value);
        bool has_semicolon();

    private:
        Expression* expression;
        bool semicolon_flag;
    };
}

#endif
