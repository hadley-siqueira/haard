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
