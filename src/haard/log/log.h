#ifndef HAARD_LOG_H
#define HAARD_LOG_H

#include <string>

namespace haard {
    typedef enum LogKind {
        LOG_ERROR,
        LOG_WARNING,
        LOG_INFO,
    } LogKind;

    class Log {
        public:
            Log();

        private:
            std::string message;

    };
};

#endif
