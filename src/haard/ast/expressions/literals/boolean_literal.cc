#include "haard/ast/expressions/literals/boolean_literal.h"

using namespace haard;

BooleanLiteral::BooleanLiteral() {
    set_kind(AST_LITERAL_BOOLEAN);
}

BooleanLiteral::BooleanLiteral(Token& token) {
    set_kind(AST_LITERAL_BOOLEAN);
    set_token(token);
}
