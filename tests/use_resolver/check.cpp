// Compiles one project and prints what the use pass reported, so that the
// golden is the diagnostics and nothing else.
//
//   check <case directory>
//
// The directory holds 'entry' naming the file to start from, the sources, and
// a 'table.tbl' when the case has imports. A case with no table is the single
// file compilation, which is most of them: the rules about what counts as a
// use are about one file, and only the qualified forms need two.
#include <haard/compilation/compilation.h>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace haard;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: check <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::filesystem::path table = directory / "table.tbl";
    Compilation compilation;

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 0;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);
    compilation.build(directory / entry);

    // the paths a diagnostic carries are this machine's, so both spellings of
    // the case directory are rewritten out before the golden sees them
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

    if (text.size() == 0) {
        std::cout << "no diagnostics\n";
        return 0;
    }

    std::cout << text;

    return 0;
}
