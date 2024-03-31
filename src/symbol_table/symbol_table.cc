#include "symbol_table/symbol_table.h"

using namespace haard;

Scope::Scope() {
    set_parent(nullptr);
}

Symbol* Scope::define(SymbolKind kind, std::string name, void* descriptor) {
    Symbol* sym;

    if (symbols.count(name) == 0) {
        sym = new Symbol();
        symbols[name] = sym;
    } else {
        sym = symbols[name];
    }

    sym->add_descriptor(descriptor);
    return sym;
}

Symbol* Scope::resolve(const std::string& name) {
    return nullptr;
}

Scope* Scope::get_parent() const {
    return parent;
}

void Scope::set_parent(Scope* newParent) {
    parent = newParent;
}
