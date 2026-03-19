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

        private:
            std::string content;
            std::filesystem::path path;
    };
}

#endif
