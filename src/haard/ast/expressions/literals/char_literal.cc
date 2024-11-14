#include "haard/ast/expressions/literals/char_literal.h"

using namespace haard;

CharLiteral::CharLiteral() {
    set_kind(AST_LITERAL_CHAR);
}

CharLiteral::CharLiteral(Token& token) {
    set_kind(AST_LITERAL_CHAR);
    set_token(token);
}
