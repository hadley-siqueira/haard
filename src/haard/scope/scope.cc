#include "haard/scope/scope.h"

using namespace haard;

void Scope::define(SymbolKind kind, const std::string& name, Ast* node) {
    Symbol* sym = new Symbol(kind, name, node);

    if (symbols.count(name) == 0) {
        std::vector<Symbol*> r;
        r.push_back(sym);
        symbols[name] = r;
    } else {
        symbols[name].push_back(sym);
    }
}

std::vector<Symbol*> Scope::resolve(std::string& name) {
    std::vector<Symbol*> r;

    return r;
}

std::vector<Symbol*> Scope::resolve_local(std::string& name) {
    std::vector<Symbol*> r;

    if (symbols.count(name) > 0) {
        return symbols[name];
    }

    return r;
}

Scope* Scope::get_parent() const {
    return parent;
}

void Scope::set_parent(Scope* parent) {
    this->parent = parent;
}
