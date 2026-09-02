// Answers every question ModuleFinder can be asked about one table, so that
// the golden reads as a resolution table.
//
//   find <case directory>
//
// The directory holds 'table.tbl' (or 'generated/table.tbl'), a 'queries.txt'
// of commands, and whatever tree those queries need:
//
//   roots            list the blocks, in the order the table declares them
//   file <path>      pick the importing root by longest prefix, and keep it
//                    for the commands after it
//   find <name>      resolve 'a.b.c' from that root
//   star <name>      resolve 'a.b.*' from that root
//   name <path>      the dotted module name of a file in that root
//   # ...            a comment
//
// Every path is printed relative to the case directory. Absolute ones would
// carry this machine's home into the goldens, and the suite would pass only
// here.
#include <haard/module_finder/module_finder.h>
#include <fstream>
#include <iostream>

using namespace haard;

static std::filesystem::path base;

static std::string show(const std::filesystem::path& path) {
    return std::filesystem::relative(path, base).string();
}

static void print_result(const FindResult& result) {
    switch (result.status) {
    case FIND_NO_ROOT:
        std::cout << "no entry for the first segment\n";
        break;

    case FIND_NO_FILE:
        std::cout << "nothing at " << show(result.path) << '\n';
        break;

    case FIND_OK:
        std::cout << show(result.path) << '\n';

        for (const std::filesystem::path& file : result.files) {
            std::cout << "    " << show(file) << '\n';
        }

        break;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: find <case directory>\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    ModuleFinder finder;

    base = std::filesystem::weakly_canonical(directory);

    // 'generated/table.tbl' when the case has one, which is the layout the
    // design proposes and the only way to write a case whose paths climb out
    // of the table's own directory
    std::filesystem::path table = directory / "generated" / "table.tbl";

    if (!std::filesystem::exists(table)) {
        table = directory / "table.tbl";
    }

    // the table is given as it was typed, so that a message about it quotes a
    // path a reader can find and not one this machine invented
    if (!finder.load(table)) {
        std::cout << "error: " << finder.get_error() << '\n';
        return 0;
    }

    std::ifstream queries(directory / "queries.txt");
    std::string line;
    u32 root = INVALID_ROOT;

    while (std::getline(queries, line)) {
        if (line.size() == 0 || line[0] == '#') {
            continue;
        }

        size_t space = line.find(' ');
        std::string command = line.substr(0, space);
        std::string argument =
            space == std::string::npos ? "" : line.substr(space + 1);

        if (command == "roots") {
            std::cout << "roots:\n";

            for (u32 i = 0; i < finder.get_root_count(); i++) {
                std::cout << "    " << i << " " << finder.get_root_name(i)
                          << " = " << show(finder.get_root_path(i)) << '\n';
            }
        } else if (command == "file") {
            root = finder.root_of_file(directory / argument);

            std::cout << "file " << argument << " -> ";

            if (root == INVALID_ROOT) {
                std::cout << "no root\n";
            } else {
                std::cout << "root " << root << " (" << finder.get_root_name(root)
                          << ")\n";
            }
        } else if (command == "find") {
            std::cout << "find " << argument << " -> ";
            print_result(finder.find(root, argument));
        } else if (command == "star") {
            std::cout << "star " << argument << ".* -> ";
            print_result(finder.find_all(root, argument));
        } else if (command == "name") {
            std::cout << "name " << argument << " -> "
                      << finder.module_name_of_file(root, directory / argument)
                      << '\n';
        } else {
            std::cerr << "unknown command: " << command << '\n';
            return 2;
        }
    }

    return 0;
}
