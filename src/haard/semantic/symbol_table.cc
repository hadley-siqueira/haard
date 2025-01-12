#include "haard/semantic/symbol_table.h"

using namespace haard;

SymbolTable::SymbolTable() {
    set_parent(nullptr);
}

SymbolTable* SymbolTable::get_parent() const {
    return parent;
}

void SymbolTable::set_parent(SymbolTable* parent) {
    this->parent = parent;
}
