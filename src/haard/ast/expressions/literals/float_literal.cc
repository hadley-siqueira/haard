#include "haard/ast/expressions/literals/float_literal.h"

using namespace haard;

FloatLiteral::FloatLiteral() {
    set_kind(AST_LITERAL_FLOAT);
}

FloatLiteral::FloatLiteral(Token& token) {
    set_kind(AST_LITERAL_FLOAT);
    set_token(token);
}
