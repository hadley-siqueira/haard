#include <haard/log/logger.h>

using namespace haard;

void Logger::log(LogKind kind, const std::string& msg) {
    Log new_log(kind, msg);
    logs.push_back(new_log);
}

void Logger::info(const std::string& msg) {
    log(LOG_INFO, msg);
}

void Logger::error(const std::string& msg) {
    log(LOG_ERROR, msg);
}

void Logger::warning(const std::string& msg) {
    log(LOG_WARNING, msg);
}

void Logger::get_logs_from(const Logger& other) {
    logs.insert(logs.end(), other.logs.begin(), other.logs.end());
}
