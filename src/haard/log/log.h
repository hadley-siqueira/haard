#ifndef HAARD_LOG_H
#define HAARD_LOG_H

#include <string>

namespace haard {
    typedef enum LogKind {
        LOG_ERROR,
        LOG_WARNING,
        LOG_INFO,
        LOG_UNKNOWN
    } LogKind;

    class Log {
        public:
            Log();
            Log(LogKind kind, const std::string& msg);

        public:
            void set_kind(LogKind kind);
            void set_message(const std::string& msg);

            LogKind get_kind();
            const std::string& get_message();

        private:
            LogKind kind;
            std::string msg;
    };
}

#endif
