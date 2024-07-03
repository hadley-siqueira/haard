#include "log/logs.h"
#include "log/logger.h"

namespace haard {
    Logger logger;

    void log_info(std::string msg) {
        logger.info(msg);
    }

    void log_error(std::string msg) {
        logger.error(msg);
    }

    std::string get_logs() {
        return logger.to_str();
    }
}
