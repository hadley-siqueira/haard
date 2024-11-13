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
