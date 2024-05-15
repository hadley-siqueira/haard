#include <iostream>

#include "scope/scope.h"

using namespace haard;

Scope::Scope() {
    set_parent(nullptr);
}

Scope::~Scope() {
    for (auto p : symbols) {
        delete p.second;
    }
}

Symbol* Scope::define(SymbolKind kind, std::string name, void* descriptor) {
    Symbol* sym;

    if (symbols.count(name) == 0) {
        sym = new Symbol(kind);
        symbols[name] = sym;
    } else {
        sym = symbols[name];
    }

    sym->add_descriptor(descriptor);
    return sym;
}

Symbol* Scope::resolve(const std::string& name) {
    Symbol* sym = resolve_in_module(name);

    if (sym != nullptr) {
        return sym;
    }

    for (int i = 0; i < siblings.size(); ++i) {
        sym = siblings[i]->resolve_in_module(name);

        if (sym != nullptr) {
            break;
        }
    }

    return nullptr;
}

Symbol* Scope::resolve_local(const std::string& name) {
    if (symbols.count(name) > 0) {
        return symbols[name];
    }

    return nullptr;
}

Symbol* Scope::resolve_in_module(const std::string& name) {
    Symbol* sym = resolve_local(name);

    if (sym != nullptr) {
        return sym;
    }

    if (get_parent()) {
        return get_parent()->resolve(name);
    }

    return nullptr;
}

Scope* Scope::get_parent() const {
    return parent;
}

void Scope::set_parent(Scope* newParent) {
    parent = newParent;
}

void Scope::inspect() {
    Scope* parent = get_parent();

    if (parent != nullptr) {
        parent->inspect();
        std::cout << " ->\n";
    }

    std::cout << "{";

    for (auto pair : symbols) {
        std::cout << pair.first << ", ";
    }

    std::cout << "}";
}
