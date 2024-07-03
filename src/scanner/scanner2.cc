#include <fstream>
#include <iostream>
#include <map>
#include "scanner/scanner2.h"
#include "token/token.h"
#include "string_pool/string_pool.h"
#include "json/json.h"

using namespace haard;

Scanner2::Scanner2() {
    line = 1;
    column = 1;
    idx = 0;
    whitespace_counter = 0;
    token_line = 0;
    token_column = 0;
    template_counter = 0;
    template_flag = false;
    tokens = new Json();
}

Json* Scanner2::read(std::string path) {
    read_to_buffer(path);
    tokens->set("path", path);

    while (has_next()) {
        get_token();
    }

    return tokens;
}

void Scanner2::read_to_buffer(std::string path) {
    char c;
    std::ifstream file;

    file.open(path.c_str());

    while (file.get(c)) {
        buffer += c;
    }

    file.close();
}

void Scanner2::get_token() {
    if (lookahead('#')) {
        skip_comment();
    } else if (lookahead('\n')) {
        advance();
        count_leading_whitespace();
    } else if (lookahead(' ')) {
        advance();
    } else if (lookahead('\"')) {
        get_double_quote_string();
    } else if (lookahead('\'')) {
        get_single_quote_string();
    } else if (is_alpha()) {
        get_keyword_or_identifier();
    } else if (is_operator()) {
        get_operator();
    } else if (is_digit()) {
        get_number();
    }
}

void Scanner2::get_keyword_or_identifier() {
    int kind;

    start_token();

    while (is_alphanum()) {
        advance();
    }

    kind = TK_ID;

    if (value_to_token_kind_map.count(value) > 0) {
        kind = value_to_token_kind_map.at(value);
    }

    create_token(kind);
    template_flag = lookahead('<');
}

void Scanner2::get_number() {
    int kind = TK_LITERAL_INTEGER;

    start_token();

    if (has_base()) {
        advance();

        if (lookahead('b')) {
            advance();

            while (is_binary_digit() || lookahead('_')) {
                advance();
            }
        } else if (lookahead('o')) {
            advance();

            while (is_octal_digit() || lookahead('_')) {
                advance();
            }
        } else if (lookahead('x')) {
            advance();

            while (is_hex_digit() || lookahead('_')) {
                advance();
            }
        } else {
            while (is_octal_digit() || lookahead('_')) {
                advance();
            }
        }
    } else {
        while (is_digit() || lookahead('_')) {
            advance();
        }

        if (lookahead('.') && !lookahead('.', 1)) {
            advance();
            kind = TK_LITERAL_DOUBLE;

            while (is_digit()) {
                advance();
            }
        }

        if (lookahead('e') || lookahead('E')) {
            kind = TK_LITERAL_DOUBLE;
            advance();

            if (lookahead('-') || lookahead('+')) {
                advance();
            }

            while (is_digit()) {
                advance();
            }
        }

        if (lookahead('f') || lookahead('F')) {
            advance();
            kind = TK_LITERAL_FLOAT;
        }
    }

    create_token(kind);
}

void Scanner2::get_operator() {
    int kind;
    std::string tmp;

    for (int i = 0; i < 4; ++i) {
        if (has_next(i)) {
            tmp += buffer[idx + i];
        }
    }

    if (template_flag || template_counter > 0) {
        template_flag = false;

        if (tmp[0] == '<') {
            start_token();
            advance();
            create_token(TK_BEGIN_TEMPLATE);
            template_counter++;
            return;
        } else if (tmp[0] == '>') {
            start_token();
            advance();
            create_token(TK_END_TEMPLATE);
            template_counter--;
            return;
        }
    }

    while (tmp.size() > 0) {
        if (value_to_token_kind_map.count(tmp) > 0) {
            kind = value_to_token_kind_map.at(tmp);
            break;
        }

        tmp.pop_back();
    }

    if (tmp.size() > 0) {
        start_token();

        for (int i = 0; i < tmp.size(); ++i) {
            advance();
        }

        create_token(kind);
    }

    // handle invalid operator
}

void Scanner2::get_single_quote_string() {
    int steps = 0;

    advance();
    start_token();

    while (!lookahead('\'')) {
        if (lookahead('\\')) {
            advance();
        }

        advance();
        steps++;
    }

    if (steps > 1) {
        create_token(TK_LITERAL_SINGLE_QUOTE_STRING);
    } else {
        create_token(TK_LITERAL_CHAR);
    }

    advance();
}

void Scanner2::get_double_quote_string() {
    advance();
    start_token();

    while (!lookahead('"')) {
        if (lookahead('\\')) {
            advance();
        }

        advance();
    }

    create_token(TK_LITERAL_DOUBLE_QUOTE_STRING);
    advance();
}

void Scanner2::skip_comment() {
    while (!lookahead('\n')) {
        advance();
    }
}

bool Scanner2::has_next(int count) {
    return idx + count < buffer.size();
}

bool Scanner2::lookahead(char c, int offset) {
    return has_next(offset) && buffer[idx + offset] == c;
}

bool Scanner2::is_alpha(int offset) {
    char c = buffer[idx + offset];

    return c >= 'a' && c <= 'z' ||
           c >= 'A' && c <= 'Z' ||
           c == '_' ||
            ((c >> 7) & 0x1) == 1;
}

bool Scanner2::is_digit(int offset) {
    return buffer[idx + offset] >= '0' && buffer[idx + offset] <= '9';
}

bool Scanner2::is_alphanum(int offset) {
    return is_alpha(offset) || is_digit(offset);
}

bool Scanner2::is_operator() {
    char c = buffer[idx];

    return c == '(' || c == ')' || c == '[' || c == ']'
        || c == '{' || c == '}' || c == '+' || c == '-'
        || c == '*' || c == '/' || c == '%' || c == '!'
        || c == '&' || c == '|' || c == '~' || c ==  '='
        || c == '>' || c == '<' || c == '^' || c == '.'
        || c == '$' || c == ':' || c == '?' || c == '@'
        || c == ',' || c == ';';
}

bool Scanner2::has_base() {
    bool flag;
    bool flag2;

    if (!has_next(2)) return false;

    char c1 = buffer[idx + 1];
    char c0 = buffer[idx];

    flag = c1 == 'o' || c1 == 'b' || c1 == 'x';
    flag2 = c1 >= '0' && c1 <= '7';

    return c0 == '0' && (flag || flag2);
}

bool Scanner2::is_binary_digit() {
    return buffer[idx] == '0' || buffer[idx] == '1';
}

bool Scanner2::is_octal_digit() {
    return buffer[idx] >= '0' && buffer[idx] <= '7';
}

bool Scanner2::is_hex_digit() {
    return buffer[idx] >= '0' && buffer[idx] <= '9' ||
           buffer[idx] >= 'a' && buffer[idx] <= 'f' ||
           buffer[idx] >= 'A' && buffer[idx] <= 'F';
}

void Scanner2::advance() {
    char c = buffer[idx];

    if (!has_next()) {
        return;
    }

    if (c == '\n') {
        line++;
        column = 1;
        whitespace_counter = 0;
        value += c;
        idx++;
    } else {
        if (((c >> 7) & 0x1) == 0) {
            advance_utf8(1);
        } else if (((c >> 5) & 0x7) == 6) {
            advance_utf8(2);
        } else if (((c >> 4) & 0xf) == 0xe) {
            advance_utf8(3);
        } else if (((c >> 3) & 0x1f) == 0x1e) {
            advance_utf8(4);
        }
    }
}

void Scanner2::advance_utf8(int counter) {
    while (has_next() && counter > 0) {
        value += buffer[idx];
        idx++;
        --counter;
    }

    column++;
}

void Scanner2::count_leading_whitespace() {
    whitespace_counter = 0;

    while (lookahead(' ')) {
        advance();
        whitespace_counter++;
    }
}

void Scanner2::start_token() {
    value = "";
    token_line = line;
    token_column = column;
}

void Scanner2::create_token(int kind) {
    Json* token = new Json();

    token->set("kind", kind);
    token->set("value", value);
    token->set("line", token_line);
    token->set("column", token_column);
    token->set("whitespace", whitespace_counter);
    /*token.set_begin(token_begin);
    token.set_end(idx);*/

    tokens->add("tokens", token);
}
