#include "symbol_table/symbol.h"

using namespace haard;

Symbol::Symbol() {

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

bool Symbol::is_function() {
    return kind == SYM_FUNCTION;
}
