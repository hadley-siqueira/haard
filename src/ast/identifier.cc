#include "ast/identifier.h"

using namespace haard;

Identifier::Identifier() {
    set_kind(AST_ID);
    alias_flag = false;
    global_alias_flag = false;
    set_overload_index(0);
}

Identifier::Identifier(Token &alias, Token &name, bool alias_flag, bool global_flag, TypeList* generics) {
    set_kind(AST_ID);
    set_name(name);
    set_alias(alias);
    set_generics(generics);
    set_overload_index(0);
    this->alias_flag = alias_flag;
    this->global_alias_flag = global_flag;
}

Identifier::~Identifier() {

}

const Token& Identifier::get_name() const {
    return get_token();
}

void Identifier::set_name(const Token& newName) {
    set_token(newName);
}

const Token& Identifier::get_alias() const {
    return alias;
}

void Identifier::set_alias(const Token& newAlias) {
    global_alias_flag = false;
    alias_flag = true;
    alias = newAlias;
}

bool Identifier::has_global_alias() {
    return global_alias_flag;
}

bool Identifier::has_alias() {
    return alias_flag;
}

TypeList* Identifier::get_generics() const {
    return generics;
}

void Identifier::set_generics(TypeList* newGenerics) {
    generics = newGenerics;
}

Symbol* Identifier::get_symbol() const {
    return symbol;
}

void Identifier::set_symbol(Symbol* newSymbol) {
    symbol = newSymbol;
}

int Identifier::get_overload_index() const {
    return overload_index;
}

void Identifier::set_overload_index(int newOverload_index) {
    overload_index = newOverload_index;
}
