#include "haard/ast/expressions/operators/unary/address_of.h"

using namespace haard;

AddressOf::AddressOf() {
    set_kind(AST_ADDRESS_OF);
}

AddressOf::AddressOf(Token& token, Expression* expression) {
    set_kind(AST_ADDRESS_OF);
    set_token(token);
    set_expression(expression);
}