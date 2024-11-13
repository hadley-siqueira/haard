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
