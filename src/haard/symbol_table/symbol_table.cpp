#include <haard/symbol_table/symbol_table.h>

using namespace haard;

SymbolTable::SymbolTable() {
    // the sentinel of each vector, so that an index of 0 reads as 'none'
    scopes.push_back(Scope{0, 0, 0});
    symbols.push_back(Symbol{0, 0, 0});
    candidates.push_back(Candidate{SYMBOL_NONE, 0, 0, 0, 0});

    // the module scope has no parent and nothing opened it
    open_scope(0, 0);
}

u32 SymbolTable::get_module_scope() {
    return 1;
}

u32 SymbolTable::open_scope(u32 parent, u32 owner) {
    scopes.push_back(Scope{parent, 0, owner});

    return (u32) scopes.size() - 1;
}

u32 SymbolTable::declare(u32 scope, u32 name, SymbolKind kind, u32 ast_node) {
    u32 symbol = find(scope, name);

    if (symbol == 0) {
        symbols.push_back(Symbol{name, 0, 0});
        symbol = (u32) symbols.size() - 1;

        // appended at the end, so a scope enumerates in declaration order
        u32 last = scopes[scope].symbols;

        if (last == 0) {
            scopes[scope].symbols = symbol;
        } else {
            while (symbols[last].sibling_or_next != 0) {
                last = symbols[last].sibling_or_next;
            }

            symbols[last].sibling_or_next = symbol;
        }
    }

    candidates.push_back(Candidate{(u8) kind, ast_node, 0, 0, 0});

    u32 candidate = (u32) candidates.size() - 1;
    u32 last = symbols[symbol].candidates;

    // and at the end here too: record 0009 resolves a bare name to the first
    // declaration that answers, so the head of this chain is the one a program
    // means when two declarations are equally good
    if (last == 0) {
        symbols[symbol].candidates = candidate;
    } else {
        while (candidates[last].next_candidate != 0) {
            last = candidates[last].next_candidate;
        }

        candidates[last].next_candidate = candidate;
    }

    return symbol;
}

u32 SymbolTable::find(u32 scope, u32 name) {
    // interning makes this a u32 comparison that never touches a byte, which
    // is the whole reason Symbol carries a name index and not a string
    for (u32 symbol = scopes[scope].symbols; symbol != 0;
         symbol = symbols[symbol].sibling_or_next) {
        if (symbols[symbol].name == name) {
            return symbol;
        }
    }

    return 0;
}

void SymbolTable::set_candidate_type(u32 candidate, u32 type) {
    candidates[candidate].type = type;
}

void SymbolTable::set_candidate_super(u32 candidate, u32 super) {
    candidates[candidate].super = super;
}

u32 SymbolTable::scope_owned_by(u32 node) {
    // scopes are few and this is asked once per field access, so a walk beats
    // a map that would have to be kept in step with the table
    for (u32 scope = 1; scope < scopes.size(); scope++) {
        if (scopes[scope].owner == node) {
            return scope;
        }
    }

    return 0;
}

u32 SymbolTable::candidate_of(u32 node) {
    for (u32 candidate = 1; candidate < candidates.size(); candidate++) {
        if (candidates[candidate].ast_node == node) {
            return candidate;
        }
    }

    return 0;
}

Scope* SymbolTable::get_scope(u32 index) {
    return &scopes[index];
}

Symbol* SymbolTable::get_symbol(u32 index) {
    return &symbols[index];
}

Candidate* SymbolTable::get_candidate(u32 index) {
    return &candidates[index];
}

u32 SymbolTable::get_scope_count() {
    return (u32) scopes.size();
}

u32 SymbolTable::get_symbol_count() {
    return (u32) symbols.size();
}

u32 SymbolTable::get_candidate_count() {
    return (u32) candidates.size();
}
