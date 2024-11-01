#include <sstream>

#include "haard/log/log.h"

using namespace haard;

Log::Log() {
    set_level(LOG_UNKNOWN);
}

std::string Log::to_str() {
    std::stringstream ss;

    switch (level) {
    case LOG_INFO:
        ss << "<blue>info:<normal> " << get_message();
        break;

    case LOG_ERROR:
        ss << "<red>error:<normal> " << get_message();
        break;

    default:
        ss << "unknown: " << get_message();
        break;
    }

    return ss.str();
}

LogLevel Log::get_level() const {
    return level;
}

void Log::set_level(LogLevel newLevel) {
    level = newLevel;
}

const std::string& Log::get_message() const {
    return message;
}

void Log::set_message(const std::string& newMessage) {
    message = newMessage;
}
