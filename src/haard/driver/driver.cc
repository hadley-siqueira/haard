#include <cstring>
#include <iostream>

#include <haard/driver/driver.h>
#include <haard/scanner/scanner.h>
#include <haard/parser/parser.h>
#include <haard/printers/pretty_printer.h>

using namespace haard;

Driver::Driver() {
    
}

void Driver::run(int argc, char** argv) {
    int idx = 1;

    for (int i = 0; i < argc; ++argv) {
        args.push_back(argv[i]);
    }

    find_files_in_arguments();

    for (auto arg : args) {
        if (arg == "-h" || arg == "--help") {
            show_help();
        } else if (arg == "--scan-json") {
            for (auto f : files) {
                Scanner scanner;

                auto tokens = scanner.get_tokens(f);
                logger.get_logs_from(scanner.get_logger());

                for (auto tk : tokens) {
                    std::cout << tk.to_json() << std::endl;
                }
            }
        } else {
            Parser parser;
            delete parser.parse_file(argv[1]);
        }

    }
}

void Driver::show_help() {
    std::cerr << "Showing help\n";
}

bool Driver::is_flag(const char* c) {
    return strcmp(cmd, c) == 0;
}

bool Driver::is_flag(const char* c1, const char* c2) {
    return is_flag(c1) || is_flag(c2);
}

void Driver::find_files_in_arguments() {
    for (auto arg : args) {
        if (arg.ends_with(".hd")) {
            files.push_back(arg);
        }
    }
}
