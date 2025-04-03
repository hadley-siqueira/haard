#include "haard/ast/types/list_type.h"

using namespace haard;

ListType::ListType() {
    set_kind(AST_TYPE_LIST);
    set_subtype(nullptr);
}

ListType::ListType(Type* subtype) {
    set_kind(AST_TYPE_LIST);
    set_subtype(subtype);
}

ListType::~ListType() {
    delete subtype;
}

Type *ListType::get_subtype() const {
    return subtype;
}

void ListType::set_subtype(Type* type) {
    this->subtype = type;
}

bool ListType::equals(Type* other) {
    if (other == nullptr) {
        return false;
    }

    if (get_kind() != other->get_kind()) {
        return false;
    }

    ListType* o2 = (ListType*) other;

    return o2->get_subtype()->equals(get_subtype());
}
