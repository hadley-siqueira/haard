#include "ir/ir.h"

using namespace haard;

IR::IR() {
    set_kind(IR_UNKNOWN);
}

IRKind IR::get_kind() const {
    return kind;
}

void IR::set_kind(IRKind newKind) {
    kind = newKind;
}
