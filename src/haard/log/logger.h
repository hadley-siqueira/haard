#ifndef HAARD_LOGGER_H
#define HAARD_LOGGER_H

#include <vector>
#include <haard/log/log.h>

namespace haard {
    class Logger {
        public:
            void log(LogKind kind, const std::string& msg);
            void info(const std::string& msg);
            void error(const std::string& msg);
            void warning(const std::string& msg);

            void get_logs_from(const Logger& other);

        private:
            std::vector<Log> logs;
    };
}

#endif
