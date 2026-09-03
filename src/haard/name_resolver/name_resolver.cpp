#include <haard/name_resolver/name_resolver.h>
#include <haard/string_table/string_table.h>

using namespace haard;

NameResolver::NameResolver() {
    compilation = nullptr;
}

void NameResolver::set_compilation(Compilation* compilation) {
    this->compilation = compilation;
}

std::vector<Candidacy> NameResolver::resolve(u32 module, u32 scope,
                                             const std::string& name) {
    std::vector<Candidacy> found;
    u32 hash = hash_name(name);
    Module* importer = compilation->get_module(module);
    SymbolTable* table = importer->get_symbols();

    // the name as this module interns it. A name the module never wrote is in
    // none of its own scopes, and the string table answers that before a
    // single scope is looked at.
    //
    // It does **not** answer for the base chain, and until 2026-09-03 this
    // loop acted as though it did: skipping the walk skipped 'gather_bases'
    // too, and a base lives in another module with a string table of its own.
    // So a field inherited across a module boundary was invisible to a bare
    // name -- record 0020 working inside one file and nowhere else. The test
    // belongs to the lookup that needs it and not to the loop
    u32 interned = importer->get_strings()->find(hash, name);
    bool shadowed = false;

    for (u32 current = scope; current != 0;
         current = table->get_scope(current)->parent) {
        u32 symbol = interned == INVALID_STRING
                         ? 0
                         : table->find(current, interned);
        u32 owner = table->get_scope(current)->owner;

        // a function joins the set and keeps the walk going; anything else
        // shadows what is outside it and the walk stops here
        if (symbol != 0) {
            gather(found, module, symbol);

            if (!only_functions(module, symbol)) {
                shadowed = true;
                break;
            }
        }

        // and, in a class body, what the classes above it declare. This step
        // belongs to the same scope as the one just looked at -- a base is
        // part of what the class means, not something outside it
        if (owner != 0 && declares_a_type(module, owner)
            && gather_bases(found, module, owner, hash, name)) {
            shadowed = true;
            break;
        }
    }

    if (shadowed) {
        return found;
    }

    // and then the imports, in the order the source wrote them, which is what
    // record 0009 resolves a bare name by. Nothing was merged at the import,
    // so each dependency is asked in its own terms
    for (const Dependency& dependency : importer->get_dependencies()) {
        u32 symbol = module_symbol(dependency.module, hash, name);

        if (symbol != 0) {
            gather(found, dependency.module, symbol);
        }
    }

    return found;
}

bool NameResolver::declares_a_type(u32 module, u32 node) {
    switch (compilation->get_module(module)->get_ast()->get_node(node)
                ->get_kind()) {
    case AST_CLASS:
    case AST_STRUCT:
    case AST_UNION:
    case AST_ENUM:
        return true;

    default:
        break;
    }

    return false;
}

bool NameResolver::gather_bases(std::vector<Candidacy>& found, u32 module,
                                u32 declaration, u32 hash,
                                const std::string& name) {
    u32 owner = module;
    u32 candidate =
        compilation->get_module(module)->get_symbols()->candidate_of(
            declaration);

    // a class reached twice is a cycle in the bases, which nothing rejects
    // yet. Stopping is not a diagnostic and does not pretend to be one; it is
    // what keeps a lookup from running forever while 1.10's checking is not
    // written
    std::vector<u32> seen;

    while (candidate != 0) {
        Module* holder = compilation->get_module(owner);
        u32 super = holder->get_symbols()->get_candidate(candidate)->super;

        if (super == INVALID_TYPE) {
            return false;
        }

        Type* entry = holder->get_types()->get_type(super);

        // a base that could not be built is nothing to look in, and it was
        // reported where it was written
        if (entry->kind != TYPE_NAMED) {
            return false;
        }

        owner = entry->module;
        candidate = entry->subject;
        holder = compilation->get_module(owner);

        u32 key = owner * 1000003 + candidate;

        for (u32 already : seen) {
            if (already == key) {
                return false;
            }
        }

        seen.push_back(key);

        // the name means nothing in the base's module until it is interned
        // there, and a module that never wrote it answers before any scope is
        // looked at
        SymbolTable* table = holder->get_symbols();
        u32 interned = holder->get_strings()->find(hash, name);
        u32 body = table->scope_owned_by(
            table->get_candidate(candidate)->ast_node);
        u32 symbol = interned == INVALID_STRING || body == 0
                         ? 0
                         : table->find(body, interned);

        if (symbol == 0) {
            continue;
        }

        gather(found, owner, symbol);

        if (!only_functions(owner, symbol)) {
            return true;
        }
    }

    return false;
}

std::vector<Candidacy> NameResolver::resolve_at_module(
        u32 module, const std::string& name) {
    SymbolTable* table = compilation->get_module(module)->get_symbols();

    return resolve(module, table->get_module_scope(), name);
}

std::vector<Candidacy> NameResolver::resolve_qualified(
        u32 module, const std::string& alias, const std::string& name) {
    std::vector<Candidacy> found;
    Module* importer = compilation->get_module(module);
    StringTable* strings = importer->get_strings();

    // an alias nobody wrote names nothing. It is interned in the importer, so
    // a miss here is an alias this file never used
    u32 wanted = strings->find(hash_name(alias), alias);

    if (wanted == INVALID_STRING) {
        return found;
    }

    u32 hash = hash_name(name);

    for (const Dependency& dependency : importer->get_dependencies()) {
        if (dependency.alias != wanted) {
            continue;
        }

        u32 symbol = module_symbol(dependency.module, hash, name);

        if (symbol != 0) {
            gather(found, dependency.module, symbol);
        }
    }

    return found;
}

void NameResolver::gather(std::vector<Candidacy>& found, u32 module,
                          u32 symbol) {
    SymbolTable* table = compilation->get_module(module)->get_symbols();

    for (u32 candidate = table->get_symbol(symbol)->candidates; candidate != 0;
         candidate = table->get_candidate(candidate)->next_candidate) {
        found.push_back(Candidacy{module, candidate});
    }
}

bool NameResolver::only_functions(u32 module, u32 symbol) {
    SymbolTable* table = compilation->get_module(module)->get_symbols();

    for (u32 candidate = table->get_symbol(symbol)->candidates; candidate != 0;
         candidate = table->get_candidate(candidate)->next_candidate) {
        if (table->get_candidate(candidate)->kind != SYMBOL_FUNCTION) {
            return false;
        }
    }

    return true;
}

u32 NameResolver::module_symbol(u32 module, u32 hash,
                                const std::string& name) {
    Module* dependency = compilation->get_module(module);

    // the probe of record 0013: a hash computed in the importer against the
    // dependency's own table, and the bytes confirm the hit
    u32 interned = dependency->get_strings()->find(hash, name);

    if (interned == INVALID_STRING) {
        return 0;
    }

    // only the module scope. Record 0008 binds what a module declares at its
    // top level and nothing it imported, so a dependency's own imports are not
    // reachable through it
    SymbolTable* table = dependency->get_symbols();

    return table->find(table->get_module_scope(), interned);
}
