#include <haard/log/log.h>

using namespace haard;

Log::Log() {
    set_kind(LOG_UNKNOWN);
}

Log::Log(LogKind kind, const std::string& msg) {
    set_kind(kind);
    set_message(msg);
}

void Log::set_kind(LogKind kind) {
    this->kind = kind;
}

void Log::set_message(const std::string& msg) {
    this->msg = msg;
}

LogKind Log::get_kind() {
    return kind;
}

const std::string& Log::get_message() {
    return msg;
}
