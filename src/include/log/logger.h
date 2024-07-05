#ifndef HAARD_LOGGER_H
#define HAARD_LOGGER_H

#include <vector>
#include "log/log.h"

namespace haard {
    class Logger {
    public:
        Logger();

    public:
        void log(std::string message);
        void info(std::string message);
        void error(std::string message);

        void clear();

        std::string to_str();

    private:
        std::vector<Log> logs;
        bool error_flag;
        bool log_info_flag;
    };
}

#endif
