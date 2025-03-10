#include <sstream>

#include "haard/ast/types/tuple_type.h"

using namespace haard;

TupleType::TupleType() {
    set_kind(AST_TYPE_TUPLE);
}

TupleType::TupleType(std::vector<Type*>& types) {
    set_kind(AST_TYPE_TUPLE);
    set_types(types);
}

TupleType::~TupleType() {
    for (auto t : types) {
        delete t;
    }
}

const std::vector<Type*>& TupleType::get_types() const {
    return types;
}

void TupleType::set_types(const std::vector<Type*>& types) {
    this->types = types;
}

bool TupleType::equals(Type* other) {
    if (other->get_kind() != get_kind()) {
        return false;
    }

    TupleType* t_other = (TupleType*) other;

    if (types.size() != t_other->types.size()) {
        return false;
    }

    for (int i = 0; i < types.size(); ++i) {
        Type* t1 = types[i];
        Type* t2 = t_other->types[i];

        if (!t1->equals(t2)) {
            return false;
        }
    }

    return true;
}
