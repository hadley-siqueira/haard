#include "symbol_table/symbol_table.h"

using namespace haard;

SymbolTable::SymbolTable() {
    set_parent(nullptr);
}

Symbol* SymbolTable::define(SymbolKind kind, std::string name, void* descriptor) {
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

Symbol* SymbolTable::resolve(const std::string& name) {
    return nullptr;
}

SymbolTable* SymbolTable::get_parent() const {
    return parent;
}

void SymbolTable::set_parent(SymbolTable* newParent) {
    parent = newParent;
}
