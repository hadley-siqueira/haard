#include <sstream>

#include "haard/ast/expressions/operators/binary/scope.h"

using namespace haard;

Scope::Scope() {
    set_kind(AST_SCOPE);
}

Scope::Scope(Token& token) {
    set_kind(AST_SCOPE);
    set_token(token);
}

std::string Scope::to_json() {
    return "scope ast json";
}

std::string Scope::to_str() {
    std::stringstream ss;

    if (get_alias()) {
        ss << get_alias()->to_str();
    }

    ss << "::" << get_name()->to_str();
    return ss.str();
}

void Scope::set_alias(Identifier* alias) {
    set_left(alias);
}

Identifier* Scope::get_alias() {
    return (Identifier*) get_left();
}

void Scope::set_name(Identifier* name) {
    set_right(name);
}

Identifier* Scope::get_name() {
    return (Identifier*) get_right();
}
