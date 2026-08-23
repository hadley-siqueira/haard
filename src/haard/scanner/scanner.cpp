#include <haard/scanner/scanner.h>
#include <cstdio>
#include <unordered_map>

using namespace haard;

TokenKind get_token_kind(const std::string& lexeme) {
    static const std::unordered_map<std::string, TokenKind> table = {
        {"import", TK_IMPORT},
        {"def", TK_DEF},
        {"class", TK_CLASS},
        {"enum", TK_ENUM},
        {"union", TK_UNION},
        {"struct", TK_STRUCT},
        {"if", TK_IF},
        {"elif", TK_ELIF},
        {"else", TK_ELSE},
        {"for", TK_FOR},
        {"while", TK_WHILE},
        {"let", TK_LET},
        {"const", TK_CONST},
        {"true", TK_TRUE},
        {"false", TK_FALSE},
        {"as", TK_AS},
        {"in", TK_IN},
        {"not", TK_NOT},
        {"return", TK_RETURN},
        {"continue", TK_CONTINUE},
        {"break", TK_BREAK},
        {"yield", TK_YIELD},
        {"this", TK_THIS},
        {"pass", TK_PASS},
        {"goto", TK_GOTO},
        {"=", TK_ASSIGNMENT},
        {"+=", TK_PLUS_ASSIGNMENT},
        {"-=", TK_MINUS_ASSIGNMENT},
        {"*=", TK_TIMES_ASSIGNMENT},
        {"/=", TK_DIVISION_ASSIGNMENT},
        {"%=", TK_MODULO_ASSIGNMENT},
        {"&=", TK_BITWISE_AND_ASSIGNMENT},
        {"|=", TK_BITWISE_OR_ASSIGNMENT},
        {"^=", TK_BITWISE_XOR_ASSIGNMENT},
        {"<<=", TK_BITWISE_LEFT_SHIFT_ASSIGNMENT},
        {">>=", TK_BITWISE_RIGHT_SHIFT_ASSIGNMENT},
        {"+", TK_PLUS},
        {"-", TK_MINUS},
        {"*", TK_TIMES},
        {"/", TK_DIVISION},
        {"++", TK_INCREMENT},
        {"--", TK_DECREMENT},
        {"==", TK_EQUAL},
        {"!=", TK_NOT_EQUAL},
        {"<", TK_LESS_THAN},
        {"<=", TK_LESS_THAN_OR_EQUAL},
        {">", TK_GREATER_THAN},
        {">=", TK_GREATER_THAN_OR_EQUAL},
        {"%", TK_MODULO},
        {".", TK_DOT},
        {"..", TK_INCLUSIVE_RANGE},
        {"...", TK_EXCLUSIVE_RANGE},
        {"->", TK_ARROW},
        {":", TK_COLON},
        {"::", TK_SCOPE},
        {"@", TK_AT},
        {"(", TK_LEFT_PARENTHESIS},
        {")", TK_RIGHT_PARENTHESIS},
        {"[", TK_LEFT_SQUARE_BRACKET},
        {"]", TK_RIGHT_SQUARE_BRACKET},
        {"{", TK_LEFT_CURLY_BRACKET},
        {"}", TK_RIGHT_CURLY_BRACKET},
        {",", TK_COMMA},
        {";", TK_SEMICOLON},
        {"!", TK_LOGICAL_NOT},
        {"&", TK_BITWISE_AND},
        {"|", TK_BITWISE_OR},
        {"~", TK_BITWISE_NOT},
        {"^", TK_BITWISE_XOR},
        {"<<", TK_BITWISE_LEFT_SHIFT},
        {">>", TK_BITWISE_RIGHT_SHIFT},
        {"&&", TK_LOGICAL_AND},
        {"||", TK_LOGICAL_OR},
        {"$", TK_DOLLAR},
        {"?", TK_QUESTION_MARK},
    };

    auto it = table.find(lexeme);

    if (it != table.end()) {
        return it->second;
    }

    return TK_UNKNOWN;
}

Scanner::Scanner() {
    tokens = nullptr;
    source_file = nullptr;
    context = nullptr;
    logger = nullptr;
    reset();
}

void Scanner::reset() {
    token_offset = 0;
    token_length = 0;
    token_line = 1;
    token_ws = 0;
    column = 1;
    line = 1;
    ws = 0;
    idx = 0;
    // zero, not one, so the first token of the file is reported as opening a
    // line: nothing precedes it
    last_end_line = 0;
    last_tab_line = 0;
    last_deep_indentation_line = 0;
    line_tab_offset = 0;
    line_start = true;
    line_has_tab = false;
    token_has_tab = false;
}

void Scanner::get_tokens(const std::filesystem::path& path) {
    reset();
    tokens->reset();
    source_file->open(path);

    while (has_next()) {
        get_token();
    }

    create_eof_token();
}

void Scanner::get_token() {
    if (is_newline()) {
        advance();
    } else if (is_comment()) {
        skip_comment();
    } else if (is_string()) {
        get_string();
    } else if (is_alpha()) {
        get_keyword_or_identifier();
    } else if (is_digit()) {
        get_number();
    } else if (is_symbol()) {
        get_symbol();
    } else if (is_operator()) {
        get_operator();
    } else if (is_whitespace()) {
        advance();
    } else {
        get_invalid_character();
    }
}

// a byte that can start no token at all. It is reported and skipped: skipping
// without a word is what the stray_characters bug used to be, and not skipping
// would call get_token on it forever
void Scanner::get_invalid_character() {
    char c = source_file->char_at(idx);
    char message[32];

    if (c >= 32 && c < 127) {
        snprintf(message, sizeof(message), "invalid character '%c'", c);
    } else {
        snprintf(message, sizeof(message), "invalid byte 0x%02X",
                 (unsigned char) c);
    }

    logger->error(idx, 1, message);
    advance();
}

void Scanner::get_keyword_or_identifier() {
    start_token();

    while (is_alphanum()) {
        advance();
    }

    end_token();
    auto lexeme = get_lexeme_from_token();
    auto kind = get_token_kind(lexeme);

    if (kind == TK_UNKNOWN) {
        kind = TK_IDENTIFIER;
    }

    create_token(kind);
}

void Scanner::get_number() {
    auto kind = TK_INTEGER_LITERAL;

    start_token();

    if (lookahead("0b")) {
        advance(2);

        if (is_binary_digit()) {
            while (is_binary_digit() || lookahead('_')) {
                advance();
            }
        } else {
            logger->error(token_offset, idx - token_offset,
                          "missing binary digits after '0b'");
            // log_error(file_id, token_offset, ERR_MISSING_BINARY_DIGITS)
        }
    } else if (lookahead("0o")) {
        advance(2);

        if (is_octal_digit()) {
            while (is_octal_digit() || lookahead('_')) {
                advance();
            }
        } else {
            logger->error(token_offset, idx - token_offset,
                          "missing octal digits after '0o'");
        }
    } else if (lookahead("0x")) {
        advance(2);

        if (is_hex_digit()) {
            while (is_hex_digit() || lookahead('_')) {
                advance();
            }
        } else {
            logger->error(token_offset, idx - token_offset,
                          "missing hexadecimal digits after '0x'");
        }
    } else {
        while (is_digit() || lookahead('_')) {
            advance();
        }

        // a '.' only opens a fraction when a digit follows it. That is what
        // keeps '1..10' a range and '1.field' a member access on an integer;
        // a trailing '1.' is an integer followed by a dot as well
        if (lookahead('.') && is_digit(1)) {
            kind = TK_FLOAT_LITERAL;
            advance();

            while (is_digit() || lookahead('_')) {
                advance();
            }
        }

        // exponent: 1e10, 2E+8, 1.5e-3. The digits are required, so the 'e' of
        // '1e' is left alone and scans as an identifier
        bool exponent = (lookahead('e') || lookahead('E'))
            && (is_digit(1)
                || ((lookahead('+', 1) || lookahead('-', 1)) && is_digit(2)));

        if (exponent) {
            kind = TK_FLOAT_LITERAL;
            advance();

            if (lookahead('+') || lookahead('-')) {
                advance();
            }

            while (is_digit() || lookahead('_')) {
                advance();
            }
        }
    }

    end_token();
    create_token(kind);
}

// literals are delimited by " or ' and may span several lines. Between single
// quotes, a lexeme holding at most one character is a char literal and anything
// longer is a string. An escape sequence and a multibyte utf8 sequence each
// count as one character
void Scanner::get_string() {
    char delimiter = source_file->char_at(idx);
    u32 counter = 0;

    if (is_template_string()) {
        get_template_string();
        return;
    }

    start_token();
    advance();

    while (has_next() && !lookahead(delimiter)) {
        if (lookahead('\\')) {
            advance();

            if (has_next()) {
                advance();
            }
        } else {
            advance();

            while (is_utf8_continuation()) {
                advance();
            }
        }

        ++counter;
    }

    if (!lookahead(delimiter)) {
        logger->error(token_offset, idx - token_offset,
                      "unterminated string literal");
        end_token();
        create_token(TK_STRING_LITERAL);
        return;
    }

    advance();
    end_token();

    if (delimiter == '\'' && counter <= 1) {
        create_token(TK_CHAR_LITERAL);
    } else {
        create_token(TK_STRING_LITERAL);
    }
}

// a string holding '${' is broken into a token sequence instead of a single
// literal, so that the interpolated expressions are scanned as regular code:
//
//   BEGIN CHUNK (INTERPOLATION_BEGIN <tokens> INTERPOLATION_END CHUNK)* END
//
// a chunk is emitted even when empty, so the sequence always alternates and
// the parser can walk it without special cases
void Scanner::get_template_string() {
    char delimiter = source_file->char_at(idx);
    u32 opening = idx;

    start_token();
    advance();
    end_token();
    create_token(TK_TEMPLATE_STRING_BEGIN);

    while (true) {
        start_token();

        while (has_next() && !lookahead(delimiter) && !is_interpolation()) {
            if (lookahead('\\')) {
                advance();

                if (has_next()) {
                    advance();
                }
            } else {
                advance();
            }
        }

        end_token();
        create_token(TK_TEMPLATE_STRING_CHUNK);

        if (!is_interpolation()) {
            break;
        }

        start_token();
        advance(2);
        end_token();
        create_token(TK_INTERPOLATION_BEGIN);

        get_interpolation();
    }

    if (!lookahead(delimiter)) {
        logger->error(opening, idx - opening, "unterminated template string");
        return;
    }

    start_token();
    advance();
    end_token();
    create_token(TK_TEMPLATE_STRING_END);
}

// scans the expression inside '${ }' as regular code. The nesting counter is
// what keeps a '}' belonging to a nested block from closing the interpolation
void Scanner::get_interpolation() {
    u32 opening = idx - 2;
    int depth = 0;

    while (has_next()) {
        if (lookahead('}')) {
            if (depth == 0) {
                break;
            }

            --depth;
        } else if (lookahead('{')) {
            ++depth;
        }

        get_token();
    }

    if (!lookahead('}')) {
        logger->error(opening, 2, "unterminated interpolation in template string");
        return;
    }

    start_token();
    advance();
    end_token();
    create_token(TK_INTERPOLATION_END);
}

// python style comment: '#' up to the end of the line. The newline itself is
// left for get_token, so that a comment behaves exactly like a blank line and
// never creates a token, which is what keeps the whitespace flag consistent
void Scanner::skip_comment() {
    while (has_next() && !is_newline()) {
        advance();
    }
}

void Scanner::get_operator() {
    std::string tmp;

    for (int i = 0; i < 4; ++i) { 
        tmp += source_file->char_at(idx + i);
    }

    while (tmp.size() > 0 && get_token_kind(tmp) == TK_UNKNOWN) {
        tmp.pop_back();
    }

    if (tmp.size() == 0) {
        logger->error(idx, 1, "unknown operator");
        // without advancing, get_token would be called again on the same
        // character forever
        advance();
        return;
    }

    auto kind = get_token_kind(tmp);

    start_token();
    advance(tmp.size());
    end_token();
    create_token(kind);
}

// ':name', ':'name with spaces'' or ':"name with spaces"'. Both quotes work and
// the other one is plain text inside. Neither form interpolates: a symbol is a
// literal, so '${' inside it is just text. A backslash escapes the next
// character, so that ':'don\'t'' closes on the right quote
void Scanner::get_symbol() {
    start_token();
    advance();

    if (lookahead('\'') || lookahead('"')) {
        char delimiter = source_file->char_at(idx);

        advance();

        while (has_next() && !lookahead(delimiter)) {
            if (lookahead('\\')) {
                advance();

                if (has_next()) {
                    advance();
                }
            } else {
                advance();

                while (is_utf8_continuation()) {
                    advance();
                }
            }
        }

        if (!lookahead(delimiter)) {
            logger->error(token_offset, idx - token_offset,
                          "unterminated symbol literal");
            end_token();
            create_token(TK_SYMBOL_LITERAL);
            return;
        }

        advance();
    } else {
        while (is_alphanum()) {
            advance();
        }
    }

    end_token();
    create_token(TK_SYMBOL_LITERAL);
}

void Scanner::set_context(Context* context) {
    this->context = context;
    this->source_file = context->get_source_file();
    this->tokens = context->get_tokens();
    this->logger = context->get_logger();
}

bool Scanner::has_next() {
    return idx < source_file->size();
};

void Scanner::start_token() {
    token_offset = idx;
    token_length = 0;
    // captured at the start because a token may span lines (multiline
    // strings), and its flag and indentation belong to the line it opens on
    token_line = line;
    token_ws = ws;
    token_has_tab = line_has_tab;
}

void Scanner::end_token() {
    token_length = idx - token_offset;

    if (token_length >= 65535) {
        logger->error(token_offset, 1, "token is longer than 65535 bytes");
    }
}

void Scanner::create_token(TokenKind kind) {
    Token token;

    // indentation is counted in spaces, so a tab adds nothing to 'ws' and a
    // tab indented block would look flush against the margin. Reported once per
    // line, and only for a line that bears a token, so that a tab on a blank or
    // comment only line stays as harmless as any other trailing whitespace
    if (token_has_tab && token_line != last_tab_line) {
        logger->error(line_tab_offset, 1, "tabs are not allowed in the indentation");
        last_tab_line = token_line;
    }

    // 'ws' only has 7 bits. Truncating it in silence would corrupt the
    // indentation comparison the parser does, so it is reported instead
    if (token_ws > 127 && token_line != last_deep_indentation_line) {
        logger->error(token_offset, token_length,
                      "indentation is deeper than 127 spaces");
        last_deep_indentation_line = token_line;
    }

    token.set_kind(kind);
    token.set_offset(token_offset);
    token.set_length(token_length);
    // compared against the line the *previous* token ended on: a token may span
    // lines, and what matters is whether a break separates the two
    token.set_newline_before(token_line != last_end_line);
    token.set_whitespace(token_ws);

    tokens->push(token);
    last_end_line = line;
}

// the stream always ends with a real TK_EOF. Its 'ws' is forced to zero, which
// makes every block the parser has open close at the end of the file with no
// special case. Its newline flag is not forced: it answers the same question as
// any other token, so a file with no trailing newline reports none
void Scanner::create_eof_token() {
    Token token;

    token.set_kind(TK_EOF);
    token.set_offset(source_file->size());
    token.set_length(0);
    token.set_newline_before(line != last_end_line);
    token.set_whitespace(0);

    tokens->push(token);
}

void Scanner::advance(int steps) {
    for (int i = 0; i < steps; ++i) {
        advance();
    }
}

void Scanner::advance() {
    char c = source_file->char_at(idx);

    if (c == '\n') {
        column = 1;
        line++;
        ws = 0;
        line_start = true;
        line_has_tab = false;
    } else if (c == ' ' && line_start) {
        ws++;
        column++;
    } else if (c == '\t' && line_start) {
        // a tab does not close the indentation and does not count towards
        // 'ws'; create_token turns the flag into a diagnostic pointing here
        if (!line_has_tab) {
            line_tab_offset = idx;
        }

        line_has_tab = true;
        column++;
    } else {
        // the first character that is not leading whitespace closes the
        // indentation of this line, freezing 'ws' until the next newline
        line_start = false;

        if (((c >> 7) & 1) == 0) {
            column++;
            //value += c;
        } else if (((c >> 6) & 0b11) == 0b10) {
            //value += c;
        } else if (((c >> 5) & 0b111) == 0b110) {
            column++;
            //value += c;
        } else if (((c >> 4) & 0b1111) == 0b1110) {
            column++;
            //value += c;
        } else if (((c >> 3) & 0b11111) == 0b11110) {
            column++;
            //value += c;
        } else {
            char buffer[32];

            snprintf(buffer, sizeof(buffer), "invalid utf8 byte 0x%02X",
                     (unsigned char) c);
            logger->error(idx, 1, buffer);
        }
    }

    ++idx;
}

bool Scanner::lookahead(char c) {
    return has_next() && source_file->char_at(idx) == c;
}

bool Scanner::lookahead(char c, int offset) {
    return source_file->char_at(idx + offset) == c;
}

bool Scanner::lookahead(const char* s) {
    int offset = 0;

    while (*s != '\0') {
        if (!lookahead(*s, offset)) {
            return false;
        }

        ++offset;
        ++s;
    }

    return true;
}

bool Scanner::is_newline() {
    return source_file->char_at(idx) == '\n';
}

// what separates tokens without being one. The '\r' of a crlf file belongs to
// the line ending, so it sits here and not among the invalid characters
bool Scanner::is_whitespace() {
    char c = source_file->char_at(idx);

    return c == ' ' || c == '\t' || c == '\r';
}

bool Scanner::is_comment() {
    return source_file->char_at(idx) == '#';
}

bool Scanner::is_string() {
    return lookahead('"') || lookahead('\'');
}

// ruby style symbol: a ':' glued to a name or to a quoted string, in either
// quote. A ':' with anything else after it stays a plain TK_COLON, and '::' is
// left to get_operator, since a second ':' is neither a name nor a quote
bool Scanner::is_symbol() {
    return lookahead(':')
        && (is_alpha(1) || lookahead('\'', 1) || lookahead('"', 1));
}

// looks ahead from the opening delimiter for a '${' before the closing one,
// without consuming anything, so that get_string can pick which form to emit
bool Scanner::is_template_string() {
    char delimiter = source_file->char_at(idx);
    size_t i = idx + 1;

    while (i < source_file->size() && source_file->char_at(i) != delimiter) {
        if (source_file->char_at(i) == '\\') {
            i += 2;
        } else if (source_file->char_at(i) == '$' && source_file->char_at(i + 1) == '{') {
            return true;
        } else {
            ++i;
        }
    }

    return false;
}

bool Scanner::is_interpolation() {
    return lookahead("${");
}

bool Scanner::is_utf8_continuation(int offset) {
    char c = source_file->char_at(idx + offset);

    return ((c >> 6) & 0b11) == 0b10;
}

// a byte >= 128 is part of a utf8 sequence, either its first byte or one of
// the continuations, and both belong to the identifier. The cast is what makes
// the test work: 'char' is signed here, so those bytes are negative
bool Scanner::is_alpha(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_'
        || ((unsigned char) c) >= 128;
}

bool Scanner::is_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '9';
}

bool Scanner::is_binary_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '1';
}

bool Scanner::is_octal_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '7';
}

bool Scanner::is_hex_digit(int offset) {
    char c = source_file->char_at(idx + offset);

    return c >= '0' && c <= '9' || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F';
}

bool Scanner::is_alphanum(int offset) {
    return is_alpha(offset) || is_digit(offset);
}

bool Scanner::is_operator(int offset) {
    char c = source_file->char_at(idx + offset);

    return c == '(' || c == ')' || c == '[' || c == ']'
        || c == '{' || c == '}' || c == '+' || c == '-'
        || c == '*' || c == '/' || c == '%' || c == '!'
        || c == '&' || c == '|' || c == '~' || c ==  '='
        || c == '>' || c == '<' || c == '^' || c == '.'
        || c == '$' || c == ':' || c == '?' || c == '@'
        || c == ',' || c == ';';
}

std::string Scanner::get_lexeme_from_token() {
    std::string r;

    for (auto i = 0; i < token_length; ++i) {
        r += source_file->char_at(token_offset + i);
    }

    return r;
}
