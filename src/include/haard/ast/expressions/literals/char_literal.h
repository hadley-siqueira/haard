#ifndef HAARD_AST_CHAR_LITERAL_H
#define HAARD_AST_CHAR_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class CharLiteral : public Literal {
    public:
        CharLiteral();
        CharLiteral(Token& token);
    };
}

#endif
