#include <haard/context/context.h>
#include <iostream>

using namespace haard;

Tokens* Context::get_tokens() {
    return &tokens;
}

SourceFile* Context::get_source_file() {
    return &source_file;
}

void Context::inspect_tokens() {
    for (auto tk : tokens.get_tokens()) {
        auto offset = tk.get_offset();
        auto length = tk.get_length();

        std::cout << offset << ":" << length << " -> '";

        for (auto i = offset; i < offset + length; ++i) {
            std::cout << source_file.char_at(i);
        }

        std::cout << "' (" << tk.get_kind() << ")\n";
    }
}
