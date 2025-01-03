#include <sstream>

#include "haard/ast/generics.h"

using namespace haard;

Generics::Generics() {
    set_kind(AST_GENERICS);
}

Generics::~Generics() {
    for (auto t : types) {
        delete t;
    }
}

void Generics::add_type(Type* type) {
    types.push_back(type);
    type->set_parent(this);
}

const std::vector<Type *>& Generics::get_types() const {
    return types;
}

void Generics::set_types(const std::vector<Type*>& types) {
    this->types = types;
}
