#include <haard/scanner/scanner.h>

using namespace haard;

void Scanner::get_tokens(std::filesystem::path& path) {
    tokens.reset();
    source_file.open(path);

    while (has_next()) {
        get_token();
    }
}

void Scanner::get_token() {
    if (lookahead('#')) {

    } else if (is_alpha()) {

    }
}

void Scanner::set_context(Context& context) {
    this->context = context;
    this->source_file = context.get_source_file();
    this->tokens = context.get_tokens();
}

bool Scanner::has_next() {
    return idx < source_file.size();
};

bool Scanner::lookahead(char c) {
    return has_next() && source_file.char_at(idx) == c;
}

bool Scanner::is_alpha(int offset) {
    char c = source_file.char_at(idx + offset);

    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_';
}

bool Scanner::is_digit(int offset) {
    char c = source_file.char_at(idx + offset);

    return c >= '0' && c <= '9';
}
