#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <haard/log/logger.h>
#include <haard/scanner/scanner.h>

namespace haard {
    class Driver {
        public:
            Driver();

        public:
            void run(int argc, char** argv);

            void show_help();

        private:
            bool is_flag(const char* c);
            bool is_flag(const char* c1, const char* c2);

        private:
            Logger logger;
            const char* cmd;
            const char* arg1;
    };
}

#endif
