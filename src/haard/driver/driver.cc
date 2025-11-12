#include <cstring>
#include <iostream>

#include <haard/driver/driver.h>
#include <haard/scanner/scanner.h>

using namespace haard;

Driver::Driver() {
    
}

void Driver::run(int argc, char** argv) {
    int idx = 0;

    while (idx < argc) {
        cmd = argv[idx];

        if (is_flag("-h", "--help")) {
            show_help();
        } else if (is_flag("--scan-json")) {
            Scanner scanner;

            if (idx + 1 >= argc) {
                logger.error("missing file to --scan flag");
                return;
            }

            auto tokens = scanner.get_tokens(argv[idx + 1]);

            for (auto tk : tokens) {
                std::cout << tk.to_json() << std::endl;
            }
        }

        ++idx;
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
