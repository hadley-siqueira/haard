#include <haard/log/log.h>

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
