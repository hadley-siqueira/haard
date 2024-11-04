#ifndef HAARD_SCOPE_H
#define HAARD_SCOPE_H

#include <string>
#include <map>
#include <vector>

#include "haard/symbol/symbol.h"

namespace haard {
    class Scope {
    public:
        void define(SymbolKind kind, const std::string &name, Ast* node);

        std::vector<Symbol*> resolve(std::string& name);
        std::vector<Symbol*> resolve_local(std::string& name);

        Scope* get_parent() const;
        void set_parent(Scope* parent);

    private:
        Scope* parent;
        std::map<std::string, std::vector<Symbol*>> symbols;
    };
}

#endif
