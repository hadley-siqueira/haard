#ifndef HAARD_SOURCE_FILE_H
#define HAARD_SOURCE_FILE_H

#include <haard/haard.h>
#include <string>
#include <vector>
#include <filesystem>

namespace haard {
    // where a token sits in the file, both 1-based. The column counts
    // characters, not bytes, so a line holding utf8 still points at the right
    // place
    struct Position {
        u32 line;
        u32 column;
    };

    class SourceFile {
        public:
            void open(const std::filesystem::path& path);

            size_t size();

            char char_at(size_t index);
            std::string get_lines_by_index(int index, int n_before, int n_after);

            // line and column are derived from an offset instead of being kept
            // in every Token: one u32 per line of the file, against 8 more
            // bytes per token. Only the diagnostics path pays for it
            Position position_of(u32 offset);

            // 1-based, without the trailing newline. Used to echo the offending
            // line under a diagnostic
            std::string line_text(u32 line);

            const std::filesystem::path& get_path();

            std::string& get_content();

        private:
            void map_lines();

        private:
            std::string content;
            std::filesystem::path path;
            std::vector<u32> line_starts;
    };
}

#endif
