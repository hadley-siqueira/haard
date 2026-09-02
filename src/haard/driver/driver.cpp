#include <haard/driver/driver.h>
#include <iostream>
#include <stdexcept>

using namespace haard;

Driver::Driver() {
    program = "hdc";
    show_help = false;
    show_tokens = false;
    show_pretty_print = false;
}

int Driver::run(int argc, char* argv[]) {
    if (!read_arguments(argc, argv)) {
        print_usage(std::cerr);
        return 2;
    }

    if (show_help) {
        print_usage(std::cout);
        return 0;
    }

    if (path.size() == 0) {
        std::cerr << program << ": no input file\n";
        print_usage(std::cerr);
        return 2;
    }

    return compile();
}

bool Driver::read_arguments(int argc, char* argv[]) {
    if (argc > 0) {
        // the name it was called by, without the directory: 'hdc: ...' reads
        // better in front of every message than the whole path does
        std::string name = argv[0];
        auto slash = name.find_last_of('/');

        program = slash == std::string::npos ? name : name.substr(slash + 1);
    }

    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];

        if (argument == "-h" || argument == "--help") {
            show_help = true;
        } else if (argument == "--tokens") {
            show_tokens = true;
        } else if (argument == "--pretty-print") {
            show_pretty_print = true;
        } else if (argument == "--roots") {
            if (i + 1 >= argc) {
                std::cerr << program << ": --roots needs the table's path\n";
                return false;
            }

            roots = argv[++i];
        } else if (argument.size() > 0 && argument[0] == '-') {
            std::cerr << program << ": unknown option '" << argument << "'\n";
            return false;
        } else if (path.size() > 0) {
            std::cerr << program << ": more than one input file\n";
            return false;
        } else {
            path = argument;
        }
    }

    return true;
}

void Driver::print_usage(std::ostream& out) {
    out << "usage: " << program << " [options] <file.hd>\n"
        << "\n"
        << "options:\n"
        << "  -h, --help          show this message\n"
        << "      --tokens        dump the token stream\n"
        << "      --pretty-print  print the source back from the ast\n"
        << "      --roots <file>  the roots table. Without it the imports of\n"
        << "                      the input file are not followed\n";
}

// Everything after the command line belongs to the Compilation: it loads the
// entry file, follows its imports and holds every module it reached. What is
// left here is where the output goes and what the exit code is
int Driver::compile() {
    if (roots.size() > 0 && !compilation.set_roots(roots)) {
        std::cerr << program << ": " << compilation.get_error() << '\n';
        return 2;
    }

    bool ok = compilation.build(path);

    // the tokens are a debugging aid and are printed even when what came
    // after the scanner failed, which is when they are wanted most
    if (show_tokens && compilation.get_module_count() > 0) {
        compilation.get_module(0)->inspect_tokens();
    }

    compilation.print_diagnostics(std::cerr);

    // a failure with no diagnostic behind it is not about the source: a file
    // that is not there, or an entry under no root block
    if (compilation.get_error().size() > 0) {
        std::cerr << program << ": " << compilation.get_error() << '\n';
        return 2;
    }

    if (!ok) {
        return 1;
    }

    if (show_pretty_print) {
        pretty_printer.set_module(compilation.get_module(0));

        if (!pretty_printer.print(std::cout)) {
            std::cerr << program << ": nothing to print, the ast is empty\n";
            return 1;
        }
    }

    return 0;
}
