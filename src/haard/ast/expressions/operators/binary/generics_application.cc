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

GenericsApplication::~GenericsApplication() {
    delete expression;
    delete generics;
}

Expression *GenericsApplication::get_expression() const {
    return expression;
}

void GenericsApplication::set_expression(Expression* expression) {
    this->expression = expression;
}

Generics* GenericsApplication::get_generics() const {
    return generics;
}

void GenericsApplication::set_generics(Generics* generics) {
    this->generics = generics;
}

std::string GenericsApplication::to_json() {
    return "generics application json";
}

std::string GenericsApplication::to_str() {
    return expression->to_str() + generics->to_str();
}
