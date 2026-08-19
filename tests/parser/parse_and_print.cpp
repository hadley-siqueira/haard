// Scans, parses and prints the tree back as source. That round trip is the
// golden test: for a file that parses, the output should be the input again;
// for one that does not, the diagnostics come first and the tree that follows
// shows what the recovery managed to keep.
#include <haard/context/context.h>
#include <haard/parser/parser.h>
#include <haard/pretty_printer/pretty_printer.h>
#include <haard/scanner/scanner.h>
#include <iostream>

using namespace haard;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: parse_and_print <file.hd>\n";
        return 2;
    }

    Context context;
    Scanner scanner;
    Parser parser;
    PrettyPrinter printer;

    scanner.set_context(&context);
    parser.set_context(&context);
    printer.set_context(&context);

    scanner.get_tokens(argv[1]);
    parser.parse();

    context.get_logger()->print(std::cout);

    if (!printer.print(std::cout)) {
        std::cout << "<no ast>\n";
    }

    return 0;
}
