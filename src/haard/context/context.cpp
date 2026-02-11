#include <haard/context/context.h>

using namespace haard;

std::vector<Token>& Context::get_tokens() {
    return tokens;
}

SourceFile& Context::get_source_file() {
    return source_file;
}

void Context::reset_tokens() {
    tokens.clear();
}
