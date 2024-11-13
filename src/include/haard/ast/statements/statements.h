#ifndef HAARD_AST_STATEMENTS_H
#define HAARD_AST_STATEMENTS_H

#include <vector>

#include "haard/ast/statements/statement.h"

namespace haard {
    class Statements : public Statement {
    public:
        Statements();
        ~Statements();

    public:
        void add_statement(Statement* statement);

        std::vector<Statement*>& get_statements();

    private:
        std::vector<Statement*> statements;
    };
}

#endif
