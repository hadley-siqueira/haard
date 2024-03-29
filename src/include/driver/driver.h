#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <string>
#include <map>

#include "log/logger.h"
#include "ast/modules.h"

namespace haard {
    class Driver {
    public:
        Driver();
        ~Driver();

        void run(int argc, char** argv);
        void exit();

    public:
        Module* parse_file(std::string path);

    private:
        bool file_exists(std::string path);

    private:
        Modules modules;
        Logger* logger;
    };
}

#endif
