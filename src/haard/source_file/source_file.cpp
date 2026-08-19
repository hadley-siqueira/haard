#include <haard/source_file/source_file.h>
#include <algorithm>
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
    this->path = path;

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

    map_lines();
};

// offset where each line begins. Line 1 is line_starts[0], and a file always
// has at least one line, even when it is empty
void SourceFile::map_lines() {
    line_starts.clear();
    line_starts.push_back(0);

    for (size_t i = 0; i < content.size(); ++i) {
        if (content[i] == '\n') {
            line_starts.push_back(i + 1);
        }
    }
}

Position SourceFile::position_of(u32 offset) {
    auto it = std::upper_bound(line_starts.begin(), line_starts.end(), offset);
    u32 index = (it - line_starts.begin()) - 1;
    u32 bol = line_starts[index];
    u32 column = 1;

    // count characters, not bytes: a utf8 continuation byte belongs to the
    // character that opened before it and must not advance the column
    for (u32 i = bol; i < offset && i < content.size(); ++i) {
        if ((((unsigned char) content[i]) & 0xC0) != 0x80) {
            ++column;
        }
    }

    return { index + 1, column };
}

std::string& SourceFile::get_content() {
    return content;
}

size_t SourceFile::size() {
    return content.size();
}

char SourceFile::char_at(size_t index) {
    if (index < content.size()) {
        return content[index];
    }

    return '\0';
}

std::string SourceFile::get_lines_by_index(int index, int n_before, int n_after) {
    std::string tmp;
    int before = index;
    int after = index;

    while (before > 0) {
        if (content[before] == '\n') {
            ++before;
            break;
        }

        --before;
    }

    while (after < content.size()) {
        if (content[after] == '\n') {
            break;
        }

        ++after;
    }

    return content.substr(before, after - before);
}
