#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <string>
#include <map>

#include "ast/modules.h"

namespace haard {
    class Driver {
    public:
        Module* parse_file(std::string path);

    private:
        Modules modules;
    };
}

#endif
