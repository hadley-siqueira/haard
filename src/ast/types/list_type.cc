#include "ast/types/list_type.h"

using namespace haard;

ListType::ListType(Type* subtype) {
    set_kind(AST_TYPE_LIST);
    set_subtype(subtype);
}
