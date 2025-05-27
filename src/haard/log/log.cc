#include <iostream>
#include <sstream>

#include <haard/log/log.h>
#include <haard/utils/utils.h>

using namespace haard;

Log::Log() {

}

Log::~Log() {

}

void Log::set_level(LogLevel level) {
    this->level = level;
}

LogLevel Log::get_level() {
    return level;
}

void Log::set_message(const std::string& message) {
    this->message = message;
}

const std::string& Log::get_message() {
    return message;
}

std::string Log::to_str() {
    std::stringstream ss;

    switch (level) {
    case LOG_WARNING:
        ss << make_purple("warning: ") << message;
        break;

    case LOG_INFO:
        ss << make_blue("info: ") << message;
        break;

    case LOG_ERROR:
        ss << make_red("error: ") << message;
        break;
    }

    return ss.str();
}
