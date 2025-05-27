#include <sstream>
#include <iostream>

#include <haard/log/logger.h>

using namespace haard;

void Logger::error(const std::string& message) {
    Log log;

    log.set_level(LOG_ERROR);
    log.set_message(message);

    logs.push_back(log);
}

const std::vector<Log>& Logger::get_logs() {
    return logs;
}

void Logger::print_logs() {
    for (auto& log : logs) {
        std::cerr << log.to_str() << '\n';
    }
}
