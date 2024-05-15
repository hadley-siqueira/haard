#include "ast/types/tuple_type.h"

using namespace haard;

TupleType::TupleType(TypeList* types) {
    set_kind(TYPE_TUPLE);
    set_types(types);
}

TypeList* TupleType::get_types() const {
    return types;
}

void TupleType::set_types(TypeList* newTypes) {
    types = newTypes;
}

bool TupleType::equals(Type* other) {
    TupleType* other_tupled = (TupleType*) other;

    if (get_kind() != other->get_kind()) {
        return false;
    }

    if (types->types_count() != other_tupled->get_types()->types_count()) {
        return false;
    }

    for (int i = 0; i < types->types_count(); ++i) {
        Type* t1 = types->get_type(i);
        Type* t2 = other_tupled->get_types()->get_type(i);

        if (!t1->equals(t2)) {
            return false;
        }
    }

    return true;
}
