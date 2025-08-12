#include <iostream>
#include <map>

#include <haard/driver/driver.h>
#include <haard/printers/pretty_printer.h>
#include <haard/parser/parser.h>

using namespace haard;

Driver::Driver() {

}

Driver::~Driver() {

}

void Driver::run(int argc, char* argv[]) {
    for (int i = 0; i < argc; ++i) {
        std::string arg(argv[i]);

        if (arg == "--help") {
            show_help();
        } else if (arg == "--pretty") {
            Parser p;
            Logger logger;
            PrettyPrinter printer;

            p.set_logger(&logger);
            auto mod = p.read(argv[1]);
            logger.print_logs();

            printer.print(mod);
            std::cout << printer.get_output() << std::endl;

            delete mod;
        }
    }
}

void Driver::show_help() {
    std::map<std::string, std::string> options;

    options["--pretty"] = "Pretty print file";
    options["--help"] = "Show this help";

    std::cerr << "Usage: hdc [options] file...\n";

    int max = 0;

    // calculate necessary spacing padding
    for (auto option : options) {
        auto len = option.first.size();
        max = max < len ? len : max;
    }

    for (auto option : options) {
        std::cerr << option.first;

        int len = max - option.first.size();
        for (int i = 0 - len; i < 4; ++i) {
            std::cerr << ' ';
        }

        std::cerr << option.second << '\n';
    }
}
