// Prints what each node turned out to be, per record 0019: its type and the
// declaration it names.
//
//   resolutions <case directory>
//
// It walks the Ast in node order and shows every node the type phase wrote
// something about, so the golden is the table itself and not this file's idea
// of it. A node with neither a type nor a declaration is not shown: the
// interesting thing is what was worked out, and a file is mostly punctuation.
//
// The node index is part of the line on purpose. It is what makes the golden
// notice a node being recorded in the wrong place -- a call's overload landing
// on the '(' instead of on the name it called.
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


// what a node shows as: the text of the token it was built from, which is the
// name for an identifier and the operator for everything else
static std::string text_of(Module* module, u32 node) {
    return std::string(
        module->get_token_value(module->get_ast()->get_node(node)->get_token()));
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: resolutions <case directory>\n";
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

        if (!module->is_parsed()) {
            continue;
        }

        ResolutionTable* resolutions = module->get_resolutions();

        std::cout << (module->get_name().size() > 0 ? module->get_name()
                                                    : "<unnamed>")
                  << ":\n";

        for (u32 node = 1; node < resolutions->get_count(); node++) {
            Resolution* found = resolutions->get(node);

            if (found->type == INVALID_TYPE && found->candidate == 0) {
                continue;
            }

            std::string line = "    " + std::to_string(node);

            while (line.size() < 10) {
                line += ' ';
            }

            line += text_of(module, node);

            while (line.size() < 24) {
                line += ' ';
            }

            line += found->type == INVALID_TYPE ? "<none>"
                                                : render(i, found->type);

            if (found->candidate != 0) {
                while (line.size() < 44) {
                    line += ' ';
                }

                Candidate* declaration = compilation.get_module(found->module)
                                             ->get_symbols()
                                             ->get_candidate(found->candidate);

                line += "-> " + declaration_name(found->module,
                                                 found->candidate);

                // three overloads of one name are three lines that would read
                // the same, so a function says which of them it is. That is
                // the whole point of recording a call's answer: the name
                // alone cannot say it
                if (declaration->kind == SYMBOL_FUNCTION) {
                    line += "  " + render(found->module, declaration->type);
                }
            }

            std::cout << line << '\n';
        }
    }

    // the type phase reports too, and the same golden should say what it
    // rejected as well as what it worked out
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
