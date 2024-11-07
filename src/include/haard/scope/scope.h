#ifndef HAARD_SCOPE2_H
#define HAARD_SCOPE2_H

#include <string>
#include <map>
#include <vector>

#include "haard/symbol/symbol.h"

namespace haard {
    class Scope2 {
    public:
        Scope2();
        ~Scope2();

    public:
        void define(SymbolKind kind, const std::string &name, Ast* node, Ast* type=nullptr);

        std::vector<Symbol*> resolve(std::string& name);
        std::vector<Symbol*> resolve_local(std::string& name);

        Scope2* get_parent() const;
        void set_parent(Scope2* parent);

    private:
        Scope2* parent;
        std::map<std::string, std::vector<Symbol*>> symbols;
    };
}

#endif
