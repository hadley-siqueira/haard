#ifndef HAARD_AST_COMPOUND_STATEMENT_H
#define HAARD_AST_COMPOUND_STATEMENT_H

#include <vector>

#include "haard/ast/statements/statement.h"

namespace haard {
    class Statements : public Statement {
    public:
        Statements();
        ~Statements();

    public:
        void add_statement(Statement* statement);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        std::vector<Statement*> statements;
    };
}

#endif
