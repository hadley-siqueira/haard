#include "haard/semantic/symbol_table.h"

using namespace haard;

SymbolTable::SymbolTable() {
    set_parent(nullptr);
}

SymbolTable::~SymbolTable() {

}

void SymbolTable::define(Symbol* symbol) {
    std::string name = symbol->get_name();

    if (symbols.count(name) > 0) {
        symbols[name].push_back(symbol);
    } else {
        std::vector<Symbol*> v;
        v.push_back(symbol);
        symbols[name] = v;
    }
}

std::vector<Symbol*> SymbolTable::resolve(std::string& name) {
    std::vector<Symbol*> r;

    r = resolve_local(name);

    if (r.size() > 0) {
        auto s = r[0];

        switch (s->get_kind()) {
        case SYM_VARIABLE:
        case SYM_FIELD:
            return r;

        default:
            break;
        }

    }

    if (get_parent()) {
        auto rr = get_parent()->resolve(name);
        r.insert(r.end(), rr.begin(), rr.end());
    }

    return r;
}

std::vector<Symbol*> SymbolTable::resolve_local(std::string &name) {
    std::vector<Symbol*> r;

    if (symbols.count(name) > 0) {
        return symbols[name];
    }

    return r;
}

SymbolTable* SymbolTable::get_parent() const {
    return parent;
}

void SymbolTable::set_parent(SymbolTable* parent) {
    this->parent = parent;
}
