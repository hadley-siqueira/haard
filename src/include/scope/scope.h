#ifndef HAARD_SCOPE_H
#define HAARD_SCOPE_H

#include <map>
#include <string>

#include "scope/symbol.h"

namespace haard {
    class Scope {
    public:
        Scope();
        ~Scope();

    public:
        Symbol* define(SymbolKind kind, std::string name, void* descriptor);

        Symbol* resolve(const std::string& name);
        Symbol* resolve_in_module(const std::string& name);

        Scope* get_parent() const;
        void set_parent(Scope* newParent);
        void inspect();

    private:
        Scope* parent;
        std::vector<Scope*> siblings;
        std::map<std::string, Symbol*> symbols;
    };
}

#endif
