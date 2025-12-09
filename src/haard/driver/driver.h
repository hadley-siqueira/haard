#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <vector>
#include <string>
#include <haard/log/logger.h>
#include <haard/scanner/scanner.h>

namespace haard {
    class Driver {
        public:
            Driver();

        public:
            void run(int argc, char** argv);

            void show_help();

            void find_files_in_arguments();

        private:
            bool is_flag(const char* c);
            bool is_flag(const char* c1, const char* c2);
            bool ends_with(const char* c);

        private:
            Logger logger;
            const char* cmd;
            const char* arg1;
            std::vector<std::string> files;
            std::vector<std::string> args;
    };
}

#endif
