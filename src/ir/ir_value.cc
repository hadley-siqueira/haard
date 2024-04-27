#include "ir/ir_value.h"

using namespace haard;

IRValueKind IRValue::get_kind() const {
    return kind;
}

void IRValue::set_kind(IRValueKind newKind) {
    kind = newKind;
}
