#include "haard/ast/types/named_type.h"

using namespace haard;

NamedType::NamedType() {
    set_kind(AST_TYPE_NAMED);
}

NamedType::NamedType(Token& alias, Token& name, std::vector<Type*>& generics) {
    set_kind(AST_TYPE_NAMED);
    set_name(name);
    set_alias(alias);
    set_generics(generics);
}

NamedType::~NamedType() {
    for (auto t : generics) {
        delete t;
    }
}

const Token& NamedType::get_alias() const {
    return alias;
}

void NamedType::set_alias(const Token& alias) {
    this->alias = alias;
}

const Token& NamedType::get_name() const {
    return name;
}

void NamedType::set_name(const Token& name) {
    this->name = name;
}

const std::vector<Type *>& NamedType::get_generics() const {
    return generics;
}

void NamedType::set_generics(const std::vector<Type*>& generics) {
    this->generics = generics;
}