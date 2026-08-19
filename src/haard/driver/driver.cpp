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
    scanner.set_context(&context);
    parser.set_context(&context);
    pretty_printer.set_context(&context);

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
        << "      --pretty-print  print the source back from the ast\n";
}

int Driver::compile() {
    if (!scan()) {
        context.get_logger()->print(std::cerr);
        return 1;
    }

    if (show_tokens) {
        context.inspect_tokens();
    }

    if (!parse()) {
        context.get_logger()->print(std::cerr);
        return 1;
    }

    if (show_pretty_print && !pretty_printer.print(std::cout)) {
        std::cerr << program << ": nothing to print, the ast is empty\n";
        return 1;
    }

    return 0;
}

// every phase follows the same shape: run it, and answer whether the ones after
// it may run. What was logged is printed by the caller, once
bool Driver::scan() {
    try {
        scanner.get_tokens(path);
    } catch (const std::exception& error) {
        // SourceFile::open throws when the file is missing or unreadable,
        // which has no offset to point at and so is not a diagnostic
        std::cerr << program << ": " << error.what() << '\n';
        return false;
    }

    return !context.get_logger()->has_errors();
}

bool Driver::parse() {
    parser.parse();

    return !context.get_logger()->has_errors();
}
