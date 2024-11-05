#include "haard/scope/scope.h"

using namespace haard;

Scope::Scope() {
    set_parent(nullptr);
}

Scope::~Scope() {
    for (auto syms : symbols) {
        for (auto s : syms.second) {
            delete s;
        }
    }
}

void Scope::define(SymbolKind kind, const std::string& name, Ast* node, Ast* type) {
    Symbol* sym = new Symbol(kind, name, node, type);

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

    r = resolve_local(name);

    if (r.size() > 0) {
        return r;
    }

    if (get_parent()) {
        return get_parent()->resolve(name);
    }

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
