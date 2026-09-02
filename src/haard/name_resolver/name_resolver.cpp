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

    // the name as this module interns it. When the module never wrote it, the
    // whole scope walk is skipped: the string table answers 'no name of mine'
    // before a single scope is looked at
    u32 interned = importer->get_strings()->find(hash, name);
    bool shadowed = false;

    for (u32 current = scope; current != 0 && interned != INVALID_STRING;
         current = table->get_scope(current)->parent) {
        u32 symbol = table->find(current, interned);

        if (symbol == 0) {
            continue;
        }

        gather(found, module, symbol);

        // a function joins the set and keeps the walk going; anything else
        // shadows what is outside it and the walk stops here
        if (!only_functions(module, symbol)) {
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
