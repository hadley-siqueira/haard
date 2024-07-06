#include <sstream>

#include "log/logger.h"
#include "utils/utils.h"

using namespace haard;

Logger::Logger() {
    error_flag = false;
    log_info_flag = true;
}

void Logger::log(std::string message) {
    Log log;

    log.set_message(message);
    Logger::logs.push_back(log);
}

void Logger::info(std::string message) {
    if (!log_info_flag)
        return;

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

bool Logger::has_error() {
    return error_flag;
}

void Logger::clear() {
    logs.clear();
}

std::string Logger::to_str() {
    std::stringstream ss;

    for (auto log : logs) {
        ss << colorify(log.to_str()) << std::endl;
    }

    return ss.str();
}
