#ifndef HAARD_AST_ELIF_STATEMENT_H
#define HAARD_AST_ELIF_STATEMENT_H

#include "haard/ast/statements/if_statement.h"
#include "haard/ast/statements/statement.h"
#include "haard/ast/expressions/expression.h"

namespace haard {
    class ElifStatement : public IfStatement {
    public:
        ElifStatement();

    };
}

#endif
