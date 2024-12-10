#ifndef HAARD_AST_ELSE_STATEMENT_H
#define HAARD_AST_ELSE_STATEMENT_H

#include "haard/ast/statements/statements.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class ElseStatement : public Statement {
    public:
        ElseStatement();
        ~ElseStatement();

    public:
        Statements* get_statements() const;
        void set_statements(Statements* statements);

        const Token& get_token() const;
        void set_token(const Token& token);

    private:
        Token token;
        Statements* statements;
    };
}

#endif
