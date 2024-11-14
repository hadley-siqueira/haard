#ifndef HAARD_AST_BOOLEAN_LITERAL_H
#define HAARD_AST_BOOLEAN_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class BooleanLiteral : public Literal {
    public:
        BooleanLiteral();
        BooleanLiteral(Token& token);
    };
}

#endif
