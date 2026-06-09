#ifndef HAARD_SOURCE_FILE_H
#define HAARD_SOURCE_FILE_H

#include <string>
#include <filesystem>

namespace haard {
    class SourceFile {
        public:
            void open(const std::filesystem::path& path);

            size_t size();

            char char_at(size_t index);
            std::string get_lines_by_index(int index, int n_before, int n_after);

        private:
            std::string content;
            std::filesystem::path path;
    };
}

#endif
