#include <haard/source_file/source_file.h>
#include <fstream>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <iostream>

using namespace haard;
namespace fs = std::filesystem;

void SourceFile::open(const std::filesystem::path& path) {
    // Open the file in binary mode for exact byte count and to prevent
    // issues with text file translations (e.g., \r\n to \n)
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }

    // Get the file size using std::filesystem
    const auto size = std::filesystem::file_size(path);

    // Pre-allocate the string to the exact size to avoid reallocations
    content.resize(size);

    // Read the entire file content into the string's buffer
    file.read(content.data(), size);

    if (!file) {
        throw std::runtime_error("Failed to read file: " + path.string());
    }
};
