#include <sstream>

#include "log/logger.h"

using namespace haard;

Logger::Logger() {
    error_flag = false;
}

void Logger::log(std::string message) {
    Log log;

    log.set_message(message);
    Logger::logs.push_back(log);
}

void Logger::info(std::string message) {
    Log log;

    log.set_level(LOG_INFO);
    log.set_message(message);
    Logger::logs.push_back(log);
}

void Logger::error(std::string message) {
    Log log;

    error_flag = true;
    log.set_level(LOG_ERROR);
    log.set_message(message);
    Logger::logs.push_back(log);
}

std::string Logger::to_str() {
    std::stringstream ss;

    for (auto log : logs) {
        ss << log.to_str() << std::endl;
    }

    return ss.str();
}
