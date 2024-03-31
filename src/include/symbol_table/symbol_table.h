#ifndef HAARD_SYMBOL_TABLE_H
#define HAARD_SYMBOL_TABLE_H

#include <map>
#include <string>

#include "symbol_table/symbol.h"

namespace haard {
    class SymbolTable {
    public:
        SymbolTable();

    public:
        Symbol* define(SymbolKind kind, std::string name, void* descriptor);

        Symbol* resolve(const std::string& name);

        SymbolTable* get_parent() const;
        void set_parent(SymbolTable *newParent);

    private:
        SymbolTable* parent;
        std::map<std::string, Symbol*> symbols;
    };
}

#endif
