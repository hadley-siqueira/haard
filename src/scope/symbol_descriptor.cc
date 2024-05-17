#include "scope/symbol_descriptor.h"

using namespace haard;

SymbolDescriptor::SymbolDescriptor() {
    set_kind(SYM_UNKNOWN);
    set_descriptor(nullptr);
}

SymbolDescriptorKind SymbolDescriptor::get_kind() {
    return kind;
}

void SymbolDescriptor::set_kind(SymbolDescriptorKind kind) {
    this->kind = kind;
}

void* SymbolDescriptor::get_descriptor() {
    return descriptor;
}

void SymbolDescriptor::set_descriptor(void* descriptor) {
    this->descriptor = descriptor;
}
