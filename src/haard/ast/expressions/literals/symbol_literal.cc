#include "haard/ast/expressions/literals/symbol_literal.h"

using namespace haard;

SymbolLiteral::SymbolLiteral() {
    set_kind(AST_LITERAL_SYMBOL);
}

SymbolLiteral::SymbolLiteral(Token& token) {
    set_kind(AST_LITERAL_SYMBOL);
    set_token(token);
}
