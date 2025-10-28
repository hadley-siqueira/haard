#include <iostream>
#include <fstream>
#include <unordered_map>

#include <haard/scanner/scanner.h>

using namespace haard;

Scanner::Scanner() {
    reset_state();
}

std::vector<Token> Scanner::get_tokens(const std::string& path) {
    reset_state();
    read_to_buffer(path);

    while (has_tokens()) {
        if (is_whitespace() || is_newline()) {
            advance();
        } else if (is_alpha()) {
            get_keyword_or_identifier();
        } else if (is_digit()) {
            get_number();
        }
    }

    return tokens;
}

void Scanner::get_keyword_or_identifier() {
    start_token();

    while (is_alphanum()) {
        advance();
    }

    if (is_keyword(value)) {

    }

    create_token();
}

void Scanner::get_number() {
    start_token();

    if (peek('0')) {
        if (peek_ahead('b', 1)) {
            advance();
            advance();

            if (is_binary_digit()) {
                while (is_binary_digit() || peek('_')) {
                    advance();
                }

                create_token();
            } else {
                std::cout << "Error on token: invalid binary literal. Expected at least an 0 or 1, but got something else\n";
            }

            return;
        } else if (peek_ahead('o', 1) || peek('_')) {
            advance();
            advance();

            while (is_octal_digit()) {
                advance();
            }

            create_token();
            return;
        } else if (peek_ahead('x', 1) || peek('_')) {
            advance();
            advance();

            while (is_hex_digit()) {
                advance();
            }

            create_token();
            return;
        }
    }

    while (is_digit() || peek('_')) {
        advance();
    }

    if (!peek('.')) {
        create_token();
        return;
    } else if (peek_ahead('.', 1)) { // avoid .. since it is the range operator and thus not part of a number
        create_token();
        return;
    } 

    advance();

    if (is_digit()) {
        while (is_digit() || peek('_')) {
            advance();
        }

        if (match('e') || match('E')) {
            if (peek('-') || peek('+')) {
                advance();
            }

            if (is_digit()) {
                while (is_digit()) {
                    advance();
                }
            } else {
                std::cout << "error: missing exponent value for number\n";
            }
        }
    }

    create_token();
}

bool Scanner::is_keyword(const std::string& v) {
    std::unordered_map<std::string, TokenKind> mapping;
    return true;
}

void Scanner::read_to_buffer(const std::string& path) {
    std::ifstream file(path);
    char c;

    while (file.get(c)) {
        buffer += c;
    }
}

void Scanner::reset_state() {
    ws = 0;
    line = 1;
    column = 1;
    offset = 0;
    idx = 0;
    buffer = "";
    line_start = true;
    tokens.clear();
}

bool Scanner::has_tokens() {
    return idx < buffer.size();
}

char Scanner::advance() {
    unsigned char c = buffer[idx];

    if (c == '\n') {
        column = 1;
        line++;
        ws = 0;
        line_start = true;
    } else if (c == ' ' && line_start) {
        ws++;
    } else if (((c >> 7) & 1) == 0) {
        column++;
        value += c;
    } else if (((c >> 6) & 0b11) == 0b10) {
        value += c;
    } else if (((c >> 5) & 0b111) == 0b110) {
        column++;
        value += c;
    } else if (((c >> 4) & 0b1111) == 0b1110) {
        column++;
        value += c;
    } else if (((c >> 3) & 0b11111) == 0b11110) {
        column++;
        value += c;
    } else {
        std::cout << "Error: unknown char = " << ((int) c) << '\n';
    }

    ++idx;
    return c;
}

bool Scanner::is_whitespace() {
    return peek(' ') || peek('\t');
}

bool Scanner::is_newline() {
    return peek('\n');
}

bool Scanner::is_alpha() {
    unsigned char c = buffer[idx];

    return peek('a', 'z') 
        || peek('A', 'Z') 
        || peek('_') 
        || c >= 128;
}

bool Scanner::is_digit() {
    return peek('0', '9');
}

bool Scanner::is_alphanum() {
    return is_alpha() || is_digit();
}

bool Scanner::is_hex_digit() {
    return is_digit() || peek('a', 'z') || peek('A', 'Z');
}

bool Scanner::is_binary_digit() {
    return peek('0') || peek('1');
}

bool Scanner::is_octal_digit() {
    unsigned char c = buffer[idx];

    return c >= '0' && c <= '7';
}

bool Scanner::match(char c) {
    if (peek(c)) {
        advance();
        return true;
    }

    return false;
}

bool Scanner::peek(char c) {
    return buffer[idx] == c;
}

bool Scanner::peek(char first, char last) {
    return buffer[idx] >= first && buffer[idx] <= last;
}

bool Scanner::peek_ahead(char c, int offset) {
    if (idx + offset < buffer.size()) {
        return buffer[idx + offset] == c;
    }

    return false;
}

void Scanner::create_token() {
    token.set_value(StringPool::get(value));
    tokens.push_back(token);
}

void Scanner::start_token() {
    token.set_line(line);
    token.set_column(column);
    token.set_whitespace(ws);
    token.set_offset(offset);
    value = "";
}
