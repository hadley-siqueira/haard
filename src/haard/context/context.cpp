#include <haard/context/context.h>

using namespace haard;

Tokens& Context::get_tokens() {
    return tokens;
}

SourceFile& Context::get_source_file() {
    return source_file;
}

