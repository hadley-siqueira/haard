#ifndef HAARD_SCOPE_H
#define HAARD_SCOPE_H

#include <string>
#include <unordered_map>
#include <vector>

#include "haard/symbol/symbol.h"

namespace haard {
    class Scope {
    public:
        Scope();
        ~Scope();

    public:
        void define(SymbolKind kind, const std::string &name, Ast* node, Ast* type=nullptr);

        std::vector<Symbol*> resolve(std::string& name);
        std::vector<Symbol*> resolve_local(std::string& name);

        Scope* get_parent() const;
        void set_parent(Scope* parent);

    private:
        Scope* parent;
        std::unordered_map<std::string, std::vector<Symbol*>> symbols;
    };
}

#endif
