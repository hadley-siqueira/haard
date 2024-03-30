#ifndef HAARD_AST_BOOLEAN_LITERAL_EXPRESSION_H
#define HAARD_AST_BOOLEAN_LITERAL_EXPRESSION_H

#include "literal.h"

namespace haard {
    class BooleanLiteral : public Literal {
    public:
        BooleanLiteral(Token& token);
    };
}

#endif
