#include <haard/log/log.h>
#include <haard/source_file/source_file.h>
#include <algorithm>

using namespace haard;

// a byte that continues a utf8 character: it belongs to the character that
// opened before it, so it takes up no column of its own
static bool is_continuation(char c) {
    return (((unsigned char) c) & 0xC0) == 0x80;
}

Log::Log(LogKind kind, u32 offset, u32 length, const std::string& message) {
    this->kind = kind;
    this->offset = offset;
    this->length = length;
    this->message = message;
}

LogKind Log::get_kind() const {
    return kind;
}

u32 Log::get_offset() const {
    return offset;
}

u32 Log::get_length() const {
    return length;
}

const std::string& Log::get_message() const {
    return message;
}

Logger::Logger() {
    source_file = nullptr;
}

void Logger::set_source_file(SourceFile* source_file) {
    this->source_file = source_file;
}

void Logger::error(u32 offset, u32 length, const std::string& message) {
    logs.push_back(Log(LOG_ERROR, offset, length, message));
}

void Logger::warning(u32 offset, u32 length, const std::string& message) {
    logs.push_back(Log(LOG_WARNING, offset, length, message));
}

bool Logger::has_errors() const {
    return count(LOG_ERROR) > 0;
}

const std::vector<Log>& Logger::get_logs() const {
    return logs;
}

u32 Logger::count(LogKind kind) const {
    u32 total = 0;

    for (auto& log : logs) {
        if (log.get_kind() == kind) {
            ++total;
        }
    }

    return total;
}

void Logger::reset() {
    logs.clear();
}

void Logger::print(std::ostream& out) {
    // by offset, so that the diagnostics of a phase that walks the file out of
    // order still read top to bottom
    std::stable_sort(logs.begin(), logs.end(), [](const Log& a, const Log& b) {
        return a.get_offset() < b.get_offset();
    });

    for (auto& log : logs) {
        print_one(out, log);
    }
}

void Logger::print_one(std::ostream& out, const Log& log) {
    switch (log.get_kind()) {
        case LOG_ERROR: out << "error: "; break;
        case LOG_WARNING: out << "warning: "; break;
        default: out << "info: "; break;
    }

    out << log.get_message() << '\n';

    if (source_file == nullptr) {
        return;
    }

    auto position = source_file->position_of(log.get_offset());
    u32 gutter = std::to_string(position.line).size();

    // rustc alignment: the arrow sits one column left of the bar, and the path
    // goes through string() because a filesystem::path streams itself quoted
    out << std::string(gutter, ' ') << "--> " << source_file->get_path().string()
        << ':' << position.line << ':' << position.column << '\n';

    print_source_line(out, log, position.line, position.column, gutter);
}

void Logger::print_source_line(std::ostream& out, const Log& log, u32 line,
                               u32 column, u32 gutter) {
    std::string text = source_file->line_text(line);
    std::string bar = std::string(gutter, ' ') + " |";

    out << bar << '\n' << line << " | " << text << '\n' << bar << ' ';

    // the padding copies the tabs of the original line instead of guessing how
    // wide they are, so the caret lands under the right character in any
    // terminal. Everything else counts as one column, utf8 included
    u32 seen = 0;

    for (u32 i = 0; i < text.size() && seen + 1 < column; ++i) {
        if (is_continuation(text[i])) {
            continue;
        }

        out << (text[i] == '\t' ? '\t' : ' ');
        ++seen;
    }

    // the caret spans the token, in characters, and never runs past the end of
    // the line: an unterminated literal reaches the end of the file, and a
    // caret that long would say nothing
    u32 carets = 0;
    auto& content = source_file->get_content();

    for (u32 i = log.get_offset();
         i < log.get_offset() + log.get_length() && i < content.size(); ++i) {
        if (content[i] == '\n') {
            break;
        }

        if (!is_continuation(content[i])) {
            ++carets;
        }
    }

    out << std::string(carets < 1 ? 1 : carets, '^') << '\n';
}
