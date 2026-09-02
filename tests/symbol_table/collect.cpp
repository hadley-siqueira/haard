// Collects the symbols of one file and draws the scope tree, so that the
// golden reads as what the module declares and where.
//
//   collect <file.hd>
//
// The shape of the dump is the shape of the data: a scope, the names in it in
// declaration order, and under each name its candidates in declaration order.
// A name with two candidates is an overload, which is the thing record 0012
// decided and nothing could show until now.
//
// A file that does not parse prints its diagnostics and nothing else, which is
// the phase gate every reader of an Ast relies on.
#include <haard/module/module.h>
#include <haard/parser/parser.h>
#include <haard/scanner/scanner.h>
#include <haard/symbol_table/symbol_collector.h>
#include <iostream>

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

// what opened this scope, by the name of the declaration that owns it. The
// module scope is owned by nothing, which is the 0 record 0013 gives it
static std::string owner_of(Module& module, u32 scope) {
    AstQuery query;
    u32 owner = module.get_symbols()->get_scope(scope)->owner;

    if (owner == 0) {
        return "the module";
    }

    // a block and a closure have no name to be called by, so they are named by
    // what they are and by the node, which is enough to tell two apart
    AstNodeKind kind = module.get_ast()->get_node(owner)->get_kind();

    const char* unnamed = kind == AST_BLOCK      ? "a block at node "
                        : kind == AST_CLOSURE    ? "a closure at node "
                        : kind == AST_FOR_EACH   ? "a for-each at node "
                        : kind == AST_FOR        ? "a for at node "
                                                 : nullptr;

    if (unnamed != nullptr) {
        return std::string(unnamed) + std::to_string(owner);
    }

    query.set_module(&module);

    return query.get_declaration_name(owner);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: collect <file.hd>\n";
        return 2;
    }

    Module module;
    Scanner scanner;
    Parser parser;
    SymbolCollector collector;

    scanner.set_module(&module);
    parser.set_module(&module);

    scanner.get_tokens(argv[1]);

    if (!module.get_logger()->has_errors()) {
        parser.parse();
    }

    if (module.get_logger()->has_errors()) {
        module.get_logger()->print(std::cout);
        return 0;
    }

    collector.set_module(&module);
    collector.collect();

    SymbolTable* table = module.get_symbols();
    StringTable* strings = module.get_strings();

    // scope 0 is the sentinel, so the walk starts at the module scope
    for (u32 scope = table->get_module_scope(); scope < table->get_scope_count();
         scope++) {
        std::cout << "scope " << scope << "  owner " << owner_of(module, scope)
                  << "  parent " << table->get_scope(scope)->parent << '\n';

        for (u32 symbol = table->get_scope(scope)->symbols; symbol != 0;
             symbol = table->get_symbol(symbol)->sibling_or_next) {
            std::cout << "  " << strings->get_text(table->get_symbol(symbol)->name)
                      << '\n';

            for (u32 candidate = table->get_symbol(symbol)->candidates;
                 candidate != 0;
                 candidate = table->get_candidate(candidate)->next_candidate) {
                std::cout << "    "
                          << kind_name(table->get_candidate(candidate)->kind)
                          << " at node "
                          << table->get_candidate(candidate)->ast_node << '\n';
            }
        }
    }

    return 0;
}
