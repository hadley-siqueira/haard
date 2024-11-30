#ifndef HAARD_AST_SYMBOL_LITERAL_H
#define HAARD_AST_SYMBOL_LITERAL_H

#include "haard/ast/expressions/literals/literal.h"

namespace haard {
    class SymbolLiteral : public Literal {
    public:
        SymbolLiteral();
        SymbolLiteral(Token& token);
    };
}

#endif
