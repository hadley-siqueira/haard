#include "haard/ast/expressions/operators/unary/parenthesis.h"

using namespace haard;

Parenthesis::Parenthesis() {
    set_kind(AST_PARENTHESIS);
}

Parenthesis::Parenthesis(Token& token, Expression* expression) {
    set_kind(AST_PARENTHESIS);
    set_token(token);
    set_expression(expression);
}