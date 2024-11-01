#ifndef HAARD_LOG_LOGS_H
#define HAARD_LOG_LOGS_H

#include <string>

namespace haard {
    void log_info(std::string msg);
    void log_error(std::string msg);
    std::string get_logs();
    void show_logs();
    bool log_has_error();
}

#endif
