#ifndef HAARD_SYMBOL_TABLE_H
#define HAARD_SYMBOL_TABLE_H

#include <vector>
#include <map>
#include <string>

#include "haard/semantic/symbol.h"

namespace haard {
    class SymbolTable {
    public:
        SymbolTable();
        ~SymbolTable();

    public:
        void define(Symbol* symbol);

        std::vector<Symbol*> resolve(std::string& name);
        std::vector<Symbol*> resolve_local(std::string& name);
        SymbolTable* get_parent() const;
        void set_parent(SymbolTable* parent);

    private:
        SymbolTable* parent;
        std::map<std::string, std::vector<Symbol*>> symbols;
    };
}

#endif
