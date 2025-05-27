#ifndef HAARD_UTILS_H
#define HAARD_UTILS_H

#include <string>

namespace haard {
    std::string make_red(const std::string& msg);
    std::string make_blue(const std::string& msg);
    std::string make_purple(const std::string& msg);
    std::string make_bold(const std::string& msg);

    std::string add_explanation (
        const std::string& msg, 
        const std::string& explanation, 
        unsigned column
    );
}

#endif
