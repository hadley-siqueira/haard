#ifndef HAARD_LOG_H
#define HAARD_LOG_H

#include <string>

namespace haard {
    typedef enum LogLevel {
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
    } LogLevel;

    class Log {
    public:
        Log();
        ~Log();

    public:
        void set_level(LogLevel level);
        LogLevel get_level();

        void set_message(const std::string& message);
        const std::string& get_message();

        std::string to_str();

    private:
        LogLevel level;
        std::string message;
    };
}

#endif
