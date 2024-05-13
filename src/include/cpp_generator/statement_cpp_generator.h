#ifndef HAARD_STATEMENT_CPP_GENERATOR_H
#define HAARD_STATEMENT_CPP_GENERATOR_H

#include <sstream>

#include "ast/statement.h"
#include "ast/compound_statement.h"
#include "ast/statements/while_statement.h"
#include "ast/expression_statement.h"

namespace haard {
    class StatementCppGenerator {
    public:
        void build(Statement* stmt);

        void build_compound_statement(CompoundStatement* stmt);
        void build_while_statement(WhileStatement* stmt);
        void build_expression_statement(ExpressionStatement* stmt);

        std::string get_output();

    private:
        std::stringstream output;
    };
}

#endif
