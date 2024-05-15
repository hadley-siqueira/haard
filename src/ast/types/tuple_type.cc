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
