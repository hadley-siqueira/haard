#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <vector>

#include <haard/ast/modules.h>
#include <haard/driver/command.h>
#include <haard/log/logger.h>

namespace haard {
    class Driver {
    public:
        Driver();
        ~Driver();

    public:
        void run(int argc, char* argv[]);
        void show_help();
        void pretty_print();

    private:
        Modules modules;
        Logger logger;
        std::vector<Command*> commands;
    };
}

#endif
