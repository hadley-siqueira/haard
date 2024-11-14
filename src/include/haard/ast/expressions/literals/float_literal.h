#ifndef HAARD_AST_FLOAT_LITERAL_H
#define HAARD_AST_FLOAT_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class FloatLiteral : public Literal {
    public:
        FloatLiteral();
        FloatLiteral(Token& token);
    };
}

#endif
