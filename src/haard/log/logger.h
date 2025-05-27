#ifndef HAARD_LOGGER_H
#define HAARD_LOGGER_H

#include <vector>
#include <string>

#include <haard/log/log.h>

namespace haard {
    class Logger {
    public:
        void error(const std::string& message);

    public:
        const std::vector<Log>& get_logs();

    private:
        std::vector<Log> logs;
    };
}

#endif
