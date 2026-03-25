#include <haard/scanner/scanner.h>
#include <iostream>

using namespace haard;

Scanner::Scanner() {
    tokens = nullptr;
    source_file = nullptr;
    context = nullptr;
    token_offset = 0;
    token_length = 0;
    idx = 0;
}

void Scanner::get_tokens(const std::filesystem::path& path) {
    tokens->reset();
    source_file->open(path);

    while (has_next()) {
        get_token();
    }
}

void Scanner::get_token() {
    if (is_alpha()) {
        get_keyword_or_identifier();
    } else {
        advance();
    }
}

void Scanner::get_keyword_or_identifier() {
    start_token();

    while (is_alphanum()) {
        advance();
    }

    end_token();
    create_token(TK_IDENTIFIER);
}

void Scanner::set_context(Context* context) {
    this->context = context;
    this->source_file = context->get_source_file();
    this->tokens = context->get_tokens();
}

bool Scanner::has_next() {
    return idx < source_file->size();
};

void Scanner::start_token() {
    token_offset = idx;
    token_length = 0;
}

void Scanner::end_token() {
    token_length = idx - token_offset;

    if (token_length >= 65535) {
        std::cout << "Error while scanning: token too long\n";
    }
}

void Scanner::create_token(TokenKind kind) {
    Token token;

    token.set_kind(kind);
    token.set_offset(token_offset);
    token.set_length(token_length);

    tokens->push(token);
}

void Scanner::advance() {
    ++idx;
}

bool Scanner::lookahead(char c) {
    return has_next() && source_file->char_at(idx) == c;
}

bool Scanner::is_alpha(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_' || c >= 128;
}

bool Scanner::is_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '9';
}

bool Scanner::is_alphanum(int offset) {
    return is_alpha(offset) || is_digit(offset);
}

