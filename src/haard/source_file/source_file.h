#ifndef HAARD_SOURCE_FILE_H
#define HAARD_SOURCE_FILE_H

#include <string>
#include <filesystem>

namespace haard {
    class SourceFile {
        public:
            void open(const std::filesystem::path& path);

        private:
            std::string content;
    };
}

#endif
