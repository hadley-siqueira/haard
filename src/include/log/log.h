#ifndef HAARD_LOG_H
#define HAARD_LOG_H

#include <string>

namespace haard {
    enum LogLevel {
        LOG_UNKNOWN,
        LOG_WARNING,
        LOG_ERROR,
        LOG_INFO
    };

    class Log {
    public:
        Log();

    public:
        std::string to_str();

        LogLevel get_level() const;
        void set_level(LogLevel newLevel);

        const std::string& get_message() const;
        void set_message(const std::string& newMessage);

    private:
        LogLevel level;
        std::string message;
    };
}

#endif
