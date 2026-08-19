#ifndef HAARD_LOG_H
#define HAARD_LOG_H

#include <haard/haard.h>
#include <string>
#include <vector>
#include <ostream>

namespace haard {
    class SourceFile;

    typedef enum LogKind {
        LOG_ERROR,
        LOG_WARNING,
        LOG_INFO,
    } LogKind;

    // one diagnostic. The position is kept as an offset and turned into
    // line:column only when it is printed, which is why no Token has to carry
    // a line of its own
    class Log {
        public:
            Log(LogKind kind, u32 offset, u32 length, const std::string& message);

        public:
            LogKind get_kind() const;
            u32 get_offset() const;
            u32 get_length() const;
            const std::string& get_message() const;

        private:
            LogKind kind;
            u32 offset;
            u32 length;
            std::string message;
    };

    // collects the diagnostics instead of printing them as they happen, so
    // that the caller decides where they go and can ask whether anything went
    // wrong before moving on to the next phase
    class Logger {
        public:
            Logger();

        public:
            void set_source_file(SourceFile* source_file);

            void error(u32 offset, u32 length, const std::string& message);
            void warning(u32 offset, u32 length, const std::string& message);

            bool has_errors() const;
            const std::vector<Log>& get_logs() const;
            u32 count(LogKind kind) const;
            void reset();

            // rustc shaped, without the notes and the suggestions:
            //
            //   error: unterminated string literal
            //    --> path/to/file.hd:4:9
            //     |
            //   4 |     a = 'no end
            //     |         ^
            void print(std::ostream& out);

        private:
            void print_one(std::ostream& out, const Log& log);
            void print_source_line(std::ostream& out, const Log& log,
                                   u32 line, u32 column, u32 gutter);

        private:
            std::vector<Log> logs;
            SourceFile* source_file;
    };
};

#endif
