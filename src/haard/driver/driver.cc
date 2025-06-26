#include <iostream>
#include <map>

#include <haard/driver/driver.h>

using namespace haard;

Driver::Driver() {
    show_help();
}

Driver::~Driver() {

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
