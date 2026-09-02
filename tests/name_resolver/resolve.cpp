// Compiles one project and then asks it what a name means, so that the golden
// is the candidate set.
//
//   resolve <case directory>
//
// The directory is a project like the compilation suite's -- a roots table, an
// 'entry' naming the file to start from, and the sources -- plus a 'queries'
// file, one question per line:
//
//   <module name> <scope> <name>
//
// where <scope> is 'module' for the module's own scope, or the name of a
// class, struct, enum or union for the scope of its body. The answer is every
// candidate, in the order the resolver gathered them, each named by the module
// it is in, its kind, and the declaration it points at.
#include <haard/ast_query/ast_query.h>
#include <haard/string_table/string_table.h>
#include <haard/name_resolver/name_resolver.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

static const char* kind_name(u8 kind) {
    switch ((SymbolKind) kind) {
    case SYMBOL_FUNCTION: return "function";
    case SYMBOL_CLASS: return "class";
    case SYMBOL_STRUCT: return "struct";
    case SYMBOL_ENUM: return "enum";
    case SYMBOL_UNION: return "union";
    case SYMBOL_FIELD: return "field";
    case SYMBOL_VARIABLE: return "variable";
    case SYMBOL_PARAM: return "param";
    case SYMBOL_GENERIC: return "generic";
    default: return "none";
    }
}

static u32 module_named(Compilation& compilation, const std::string& name) {
    for (u32 i = 0; i < compilation.get_module_count(); i++) {
        if (compilation.get_module(i)->get_name() == name) {
            return i;
        }
    }

    return INVALID_ROOT;
}

// Three ways to name the scope a question is asked from:
//
//   module      the module's own scope
//   <name>      the scope owned by that declaration -- a class, or a function
//   @<name>     the scope that declares <name>, which is how a block is
//               reached: a block has no name of its own, and naming it by
//               something it declares survives a case being edited in a way a
//               scope number would not
static u32 scope_named(Compilation& compilation, u32 module,
                       const std::string& name) {
    Module* found = compilation.get_module(module);
    SymbolTable* table = found->get_symbols();
    StringTable* strings = found->get_strings();
    AstQuery query;

    if (name == "module") {
        return table->get_module_scope();
    }

    if (name[0] == '@') {
        std::string declared = name.substr(1);
        u32 interned = strings->find(hash_name(declared), declared);

        for (u32 scope = 1;
             interned != INVALID_STRING && scope < table->get_scope_count();
             scope++) {
            if (table->find(scope, interned) != 0) {
                return scope;
            }
        }

        return 0;
    }

    query.set_module(found);

    for (u32 scope = 1; scope < table->get_scope_count(); scope++) {
        u32 owner = table->get_scope(scope)->owner;

        if (owner != 0 && query.get_declaration_name(owner) == name) {
            return scope;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: resolve <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "table.tbl";
    Compilation compilation;
    NameResolver resolver;

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);
    compilation.build(directory / entry);
    resolver.set_compilation(&compilation);

    std::ifstream queries(directory / "queries");
    std::string line;

    while (std::getline(queries, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        std::istringstream fields(line);
        std::string module_name;
        std::string scope_name;
        std::string name;

        fields >> module_name >> scope_name >> name;

        std::cout << module_name << ' ' << scope_name << " -> " << name
                  << '\n';

        u32 module = module_named(compilation, module_name);

        if (module == INVALID_ROOT) {
            std::cout << "  no module named " << module_name << '\n';
            continue;
        }

        u32 scope = scope_named(compilation, module, scope_name);

        if (scope == 0) {
            std::cout << "  no scope named " << scope_name << '\n';
            continue;
        }

        // 'alias::name' and '::name' are written in the query exactly as a
        // programmer writes them, and the scope field is ignored for both:
        // one names its imports and the other starts at the module scope
        std::vector<Candidacy> found;
        size_t colons = name.find("::");

        if (colons == std::string::npos) {
            found = resolver.resolve(module, scope, name);
        } else if (colons == 0) {
            found = resolver.resolve_at_module(module, name.substr(2));
        } else {
            found = resolver.resolve_qualified(module, name.substr(0, colons),
                                               name.substr(colons + 2));
        }

        if (found.size() == 0) {
            std::cout << "  nothing declares it\n";
            continue;
        }

        for (const Candidacy& candidacy : found) {
            Module* owner = compilation.get_module(candidacy.module);
            Candidate* candidate =
                owner->get_symbols()->get_candidate(candidacy.candidate);

            // the node is printed because two candidates of one name in one
            // module are otherwise the same line, and their order is the
            // whole point of record 0009
            // No name is printed per candidate, and that is the data model
            // and not an omission: record 0013 puts the name on the Symbol,
            // and a candidate is only ever reached through one. Two loop
            // variables of one 'for ... in' point at the same node and are
            // told apart by the symbols above them, exactly as a tuple
            // binding's names are
            std::cout << "  " << owner->get_name() << ' '
                      << kind_name(candidate->kind) << " at node "
                      << candidate->ast_node << '\n';
        }
    }

    return 0;
}
