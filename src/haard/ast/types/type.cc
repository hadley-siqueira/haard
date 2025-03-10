#include "haard/ast/types/type.h"

using namespace haard;


bool Type::equals(Type* other) {
    return get_kind() == other->get_kind();
}
