#ifndef HAARD_AST_STRING_LITERAL_H
#define HAARD_AST_STRING_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class StringLiteral : public Literal {
    public:
        StringLiteral();
        StringLiteral(Token& token);
    };
}

#endif
