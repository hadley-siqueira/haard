#ifndef HAARD_AST_STATEMENTS_H
#define HAARD_AST_STATEMENTS_H

#include <vector>

#include <haard/ast/statement.h>

namespace haard {
    class Statements : public Statement {
    public:
        void add_statement(Statement* statement);

    private:
        std::vector<Statement*> statements;
    };
}

#endif
