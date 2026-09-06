// Compiles one project and prints the entry module's tree BACK AS HAARD,
// after record 0032's lowering pass has run over it.
//
//   show <case directory>
//
// The golden is therefore the sugar taken apart, written in the language it
// was written in. That is not a convenience: record 0025 chose an Ast -> Ast
// pass over an intermediate representation with exactly this argument -- the
// PrettyPrinter is already an oracle for a tree, so a pass that rewrites one
// can be read instead of inferred.
//
// The diagnostics come after it, so a case that is refused shows both what
// was said and what the tree was left as.
#include <haard/compilation/compilation.h>
#include <haard/pretty_printer/pretty_printer.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: show <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "table.tbl";
    Compilation compilation;
    PrettyPrinter printer;

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);
    compilation.build(directory / entry);

    printer.set_module(compilation.get_module(0));
    printer.print(std::cout);

    // this machine's paths are in every diagnostic, in both spellings the
    // case can be reached by, and neither belongs in a golden
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
        std::cout << "\n--- diagnostics\n" << text;
    }

    return 0;
}
