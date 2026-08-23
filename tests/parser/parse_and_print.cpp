// Scans and parses a file, then checks the result three ways at once:
//
//   1. the diagnostics it reported;
//   2. the source the pretty printer writes back from the tree;
//   3. the shape of the tree itself.
//
// All three go into the golden file. The third is the one that earns its keep:
// the printed source is an in-order walk of the leaves and the operators, so
// every tree over the same sequence prints the same text and a parser that
// folded 'a + b * c' the wrong way would still print 'a + b * c'. The dump is
// what makes precedence, associativity and the token behind each node visible.
//
// Then it closes the loop: the printed source is parsed again and the two trees
// are compared. Printing a tree and reading the result back has to give the
// same tree, or the printer and the parser disagree about the language. The
// comparison is on the dumps, so it is structural — node indices and token
// offsets differ between the two runs and must not count.
//
//   parse_and_print <file.hd> [reprint.hd]
//
// Without the second argument the round trip is skipped, which is what makes
// the program usable by hand on a file the runner did not stage.
#include <haard/context/context.h>
#include <haard/parser/parser.h>
#include <haard/pretty_printer/pretty_printer.h>
#include <haard/scanner/scanner.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

// what a node is called in the dump. A kind with no name here would show up as
// a number, which is the same reason the printer has a default case
static std::string name_of(AstNodeKind kind) {
    switch (kind) {
        case AST_MODULE: return "module";
        case AST_IMPORT: return "import";
        case AST_IMPORT_PATH: return "import_path";
        case AST_IMPORT_PATH_SEGMENT: return "import_path_segment";
        case AST_IMPORT_ALIAS: return "import_alias";
        case AST_LET_DECLARATION: return "let_declaration";
        case AST_CONST_DECLARATION: return "const_declaration";
        case AST_PARAM: return "param";
        case AST_BINDING: return "binding";
        case AST_BINDING_NAME: return "binding_name";
        case AST_BINDING_TYPE: return "binding_type";
        case AST_BINDING_EXPRESSION: return "binding_expression";
        case AST_PLUS: return "plus";
        case AST_MINUS: return "minus";
        case AST_TIMES: return "times";
        case AST_DIVISION: return "division";
        case AST_MODULO: return "modulo";
        case AST_SCOPE: return "scope";
        case AST_PARENTHESIS: return "parenthesis";
        case AST_IDENTIFIER: return "identifier";
        case AST_UNKNOWN: break;
    }

    return "<unnamed kind " + std::to_string((int) kind) + ">";
}

// whether the node's token index means anything. It has to be asked by kind,
// because a node with no token of its own carries a 0 and 0 is also the index
// of the first token of the file: the two are indistinguishable from the node
// alone. A kind missing from this list only loses its lexeme in the dump, it
// never invents one, so this is the safe direction to be wrong in.
static bool carries_a_token(AstNodeKind kind) {
    switch (kind) {
        case AST_IMPORT:
        case AST_IMPORT_PATH_SEGMENT:
        case AST_IMPORT_ALIAS:
        case AST_LET_DECLARATION:
        case AST_CONST_DECLARATION:
        case AST_PARAM:
        case AST_PLUS:
        case AST_MINUS:
        case AST_TIMES:
        case AST_DIVISION:
        case AST_MODULO:
        case AST_SCOPE:
        case AST_PARENTHESIS:
        case AST_IDENTIFIER:
            return true;

        default:
            break;
    }

    return false;
}

// one node per line, two spaces per level. The indentation is what records the
// shape, so comparing two dumps as strings compares the two trees
static void dump_node(std::ostream& out, Context& context, u32 node, u32 depth) {
    while (node != 0) {
        AstNode* current = context.get_ast()->get_node(node);
        AstNodeKind kind = current->get_kind();

        out << std::string(depth * 2, ' ') << name_of(kind);

        if (carries_a_token(kind)) {
            out << " '" << context.get_token_value(current->get_token()) << "'";
        }

        out << '\n';

        dump_node(out, context, current->get_children(), depth + 1);
        node = current->get_sibling();
    }
}

static std::string dump_of(Context& context) {
    std::stringstream out;

    dump_node(out, context, context.get_ast()->get_root(), 0);

    return out.str();
}

// scans and parses one file into a context of its own. Two of these never share
// anything, which is the point: the second tree has to stand on its own tokens
static void read(Context& context, const std::filesystem::path& path) {
    Scanner scanner;
    Parser parser;

    scanner.set_context(&context);
    parser.set_context(&context);

    scanner.get_tokens(path);
    parser.parse();
}

static bool write_file(const std::filesystem::path& path,
                       const std::string& content) {
    std::ofstream out(path);

    out << content;

    return out.good();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: parse_and_print <file.hd> [reprint.hd]\n";
        return 2;
    }

    Context context;
    PrettyPrinter printer;
    std::stringstream source;

    read(context, argv[1]);
    printer.set_context(&context);

    context.get_logger()->print(std::cout);

    if (!printer.print(source)) {
        std::cout << "<no ast>\n";
        return 0;
    }

    std::cout << source.str();
    std::cout << "--- tree\n" << dump_of(context);

    if (argc < 3) {
        return 0;
    }

    // the loop: print the tree as source, read that back, and the two trees
    // have to be the same one
    if (!write_file(argv[2], source.str())) {
        std::cout << "--- round trip: could not write the printed source\n";
        return 1;
    }

    Context reparsed;

    read(reparsed, argv[2]);

    // the printed source is what the parser kept, so it must parse cleanly even
    // when the file it came from did not
    if (reparsed.get_logger()->has_errors()) {
        std::cout << "--- round trip: the printed source did not parse\n";
        reparsed.get_logger()->print(std::cout);
        return 1;
    }

    std::string before = dump_of(context);
    std::string after = dump_of(reparsed);

    if (before != after) {
        std::cout << "--- round trip: the tree changed\n" << after;
        return 1;
    }

    std::cout << "--- round trip: ok\n";

    return 0;
}
