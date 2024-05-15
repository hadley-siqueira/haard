#include "scope/symbol.h"

using namespace haard;

Symbol::Symbol() {

}

Symbol::Symbol(SymbolKind kind) {
    set_kind(kind);
}

SymbolKind Symbol::get_kind() const {
    return kind;
}

void Symbol::set_kind(SymbolKind newKind) {
    kind = newKind;
}

void Symbol::add_descriptor(void* descriptor) {
    descriptors.push_back(descriptor);
}

int Symbol::descriptors_count() {
    return descriptors.size();
}

void* Symbol::get_descriptor(int idx) {
    if (idx < descriptors_count()) {
        return descriptors[idx];
    }

    return nullptr;
}

bool Symbol::is_function() {
    return kind == SYM_FUNCTION;
}
