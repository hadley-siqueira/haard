#include "ast/unary_operators/address_of.h"

using namespace haard;

AddressOf::AddressOf(Expression* expression) {
    set_kind(AST_ADDRESS_OF);
    set_expression(expression);
}

AddressOf::AddressOf(Token& token, Expression* expression) {
    set_kind(AST_ADDRESS_OF);
    set_expression(expression);
    set_token(token);
}
