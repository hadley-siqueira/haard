// Prints the type the collector gave every declaration of a project, rendered
// back as source, so that the golden reads as what the file meant.
//
//   types <case directory>
//
// Rendering back to the written syntax is the check that matters: 'i32[10]*[5]'
// only comes out again if the nesting the parser built was walked in the right
// order, and record 0016 says the type table adds no nesting of its own.
//
// It reads the table the compilation filled and builds nothing of its own, so
// what the golden shows is the phase and not this file's idea of it.
#include <haard/type_table/type_collector.h>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace haard;

static Compilation compilation;

static const char* builtin_name(u32 code) {
    static const char* names[] = {
        "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64",
        "f32", "f64", "bool", "void", "char"
    };

    return code < BUILTIN_COUNT ? names[code] : "?";
}

// the name of the declaration a named or generic type points at. A generic
// parameter is its own identifier; everything else wraps its name
static std::string declaration_name(u32 module, u32 candidate) {
    Module* owner = compilation.get_module(module);
    Candidate* found = owner->get_symbols()->get_candidate(candidate);
    AstQuery query;

    if (found->kind == SYMBOL_GENERIC) {
        return std::string(owner->get_token_value(
            owner->get_ast()->get_node(found->ast_node)->get_token()));
    }

    query.set_module(owner);

    // a case with no roots table has unnamed modules, and prefixing an empty
    // name with a dot would be noise in every single file golden
    std::string prefix = owner->get_name();

    return (prefix.size() > 0 ? prefix + "." : "")
         + query.get_declaration_name(found->ast_node);
}

static std::string render(u32 module, u32 type) {
    TypeTable* table = compilation.get_module(module)->get_types();
    Type* entry = table->get_type(type);
    std::vector<u32> arguments = table->get_arguments(type);
    std::string out;

    switch ((TypeKind) entry->kind) {
    case TYPE_BUILTIN:
        return builtin_name(entry->subject);

    case TYPE_POINTER:
        return render(module, arguments[0]) + "*";

    case TYPE_REFERENCE:
        return render(module, arguments[0]) + "&";

    case TYPE_ARRAY:
        return render(module, arguments[0]) + "["
             + (entry->subject == NO_LENGTH ? ""
                                            : std::to_string(entry->subject))
             + "]";

    case TYPE_LIST:
        return "[" + render(module, arguments[0]) + "]";

    case TYPE_HASH:
        return "{" + render(module, arguments[0]) + ": "
             + render(module, arguments[1]) + "}";

    case TYPE_TUPLE:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? ", " : "") + render(module, arguments[i]);
        }

        return "(" + out + ")";

    case TYPE_FUNCTION:
        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? " -> " : "") + render(module, arguments[i]);
        }

        return out;

    case TYPE_GENERIC:
        return declaration_name(module, entry->subject);

    case TYPE_NAMED:
        out = declaration_name(entry->module, entry->subject);

        if (arguments.size() == 0) {
            return out;
        }

        out += "<";

        for (u32 i = 0; i < arguments.size(); i++) {
            out += (i > 0 ? ", " : "") + render(module, arguments[i]);
        }

        return out + ">";

    default:
        break;
    }

    return "<none>";
}

// the name of a declaration, and a generic parameter is its own identifier
static std::string name_of(Module* owner, u32 node, u8 kind) {
    AstQuery query;

    if (kind == SYMBOL_GENERIC) {
        return std::string(owner->get_token_value(
            owner->get_ast()->get_node(node)->get_token()));
    }

    query.set_module(owner);

    return query.get_declaration_name(node);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: types <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "table.tbl";

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);
    compilation.build(directory / entry);

    for (u32 i = 0; i < compilation.get_module_count(); i++) {
        Module* module = compilation.get_module(i);
        SymbolTable* symbols = module->get_symbols();

        if (!module->is_parsed()) {
            continue;
        }

        std::cout << (module->get_name().size() > 0 ? module->get_name()
                                                    : "<unnamed>")
                  << ":\n";

        // every scope, every name, every candidate -- the order the collector
        // walked them in, so a golden line moves when a scope does
        for (u32 scope = 1; scope < symbols->get_scope_count(); scope++) {
            for (u32 symbol = symbols->get_scope(scope)->symbols; symbol != 0;
                 symbol = symbols->get_symbol(symbol)->sibling_or_next) {
                for (u32 candidate = symbols->get_symbol(symbol)->candidates;
                     candidate != 0;
                     candidate =
                         symbols->get_candidate(candidate)->next_candidate) {
                    Candidate* found = symbols->get_candidate(candidate);

                    std::cout << "    "
                              << name_of(module, found->ast_node, found->kind)
                              << "  " << render(i, found->type) << '\n';
                }
            }
        }

        std::cout << "  " << module->get_types()->get_count()
                  << " types interned\n";
    }

    // the type phase reports too, and the same golden should say what it
    // rejected as well as what it worked out. Paths are this machine's, so
    // both spellings of the case directory are rewritten out
    std::ostringstream diagnostics;
    std::string text;
    std::string prefixes[] = {
        std::filesystem::weakly_canonical(directory).string() + "/",
        directory.string() + "/"
    };

    compilation.print_diagnostics(diagnostics);
    text = diagnostics.str();

    for (const std::string& prefix : prefixes) {
        for (size_t at = text.find(prefix); at != std::string::npos;
             at = text.find(prefix, at)) {
            text.erase(at, prefix.size());
        }
    }

    if (text.size() > 0) {
        std::cout << "diagnostics:\n" << text;
    }

    return 0;
}
