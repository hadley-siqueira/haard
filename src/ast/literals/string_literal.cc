#include "ast/literals/string_literal.h"

using namespace haard;

StringLiteral::StringLiteral(Token& token) {
    set_kind(AST_LITERAL_STRING);
    set_token(token);
}
