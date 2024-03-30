#include "ast/literals/boolean_literal.h"

using namespace haard;

BooleanLiteral::BooleanLiteral(Token& token) {
    set_kind(EXPR_LITERAL_BOOLEAN);
    set_token(token);
}
