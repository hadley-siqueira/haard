#ifndef HAARD_AST_INTEGER_LITERAL_H
#define HAARD_AST_INTEGER_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class IntegerLiteral : public Literal {
    public:
        IntegerLiteral();
        IntegerLiteral(Token& token);
    };
}

#endif
