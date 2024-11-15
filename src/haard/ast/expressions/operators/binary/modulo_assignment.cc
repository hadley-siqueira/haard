#include "haard/ast/expressions/operators/binary/modulo_assignment.h"

using namespace haard;

ModuloAssignment::ModuloAssignment() {
    set_kind(AST_MODULO_ASSIGNMENT);
}

ModuloAssignment::ModuloAssignment(Token& token, Expression* left, Expression* right) {
    set_kind(AST_MODULO_ASSIGNMENT);
    set_token(token);
    set_left(left);
    set_left(right);
}