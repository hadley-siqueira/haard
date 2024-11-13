#include "haard/ast/expressions/operators/binary/generics_application.h"

using namespace haard;

GenericsApplication::GenericsApplication() {
    set_kind(AST_GENERIC_APPLICATION);
    set_expression(nullptr);
    set_generics(nullptr);
}

GenericsApplication::GenericsApplication(Expression* expression, Generics* generics) {
    set_kind(AST_GENERIC_APPLICATION);
    set_expression(expression);
    set_generics(generics);
}

Expression* GenericsApplication::get_expression() const {
    return get_left();
}

void GenericsApplication::set_expression(Expression* expression) {
    set_left(expression);
}

Generics* GenericsApplication::get_generics() const {
    return (Generics*) get_right();
}

void GenericsApplication::set_generics(Generics* generics) {
    set_right(generics);
}
