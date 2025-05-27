#include <iostream>
#include <sstream>
#include <fstream>

#include <haard/utils/utils.h>

namespace haard{
    std::string make_red(const std::string& msg) {
        std::string out;
        out = "\033[31m" + msg + "\033[0m";
        return out;
    }

    std::string make_purple(const std::string& msg) {
        std::string out;
        out = "\033[35m" + msg + "\033[0m";
        return out;
    }

    std::string make_blue(const std::string& msg) {
        std::string out;
        out = "\033[34m" + msg + "\033[0m";
        return out;
    }

    std::string make_bold(const std::string& msg) {
        std::string out;
        out = "\033[1m" + msg + "\033[0m";
        return out;
    }

    std::string add_explanation (
        const std::string& msg, 
        const std::string& explanation, 
        unsigned column
    ) {
        std::stringstream ss;

        ss << msg << '\n';

        for (int i = 1; i < column; ++i) {
            ss << ' ';
        }

        ss << "^\n";

        for (int i = 1; i < column; ++i) {
            ss << ' ';
        }

        for (auto c : explanation) {
            ss << c;

            if (c == '\n') {
                for (int i = 1; i < column; ++i) {
                    ss << ' ';
                }
            }
        }

        return ss.str();
    }
}
