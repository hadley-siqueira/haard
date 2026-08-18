#include <haard/token_stream/token_stream.h>

using namespace haard;

TokenStream::TokenStream() {
    eof.set_kind(TK_EOF);
}

void TokenStream::reset() {
    tokens.clear();
}

void TokenStream::push(Token& token) {
    tokens.push_back(token);
}

size_t TokenStream::size() {
    return tokens.size();
}

std::vector<Token>& TokenStream::get_tokens() {
    return tokens;
}

Token& TokenStream::get_token(u32 index) {
    if (index < tokens.size()) {
        return tokens[index];
    }

    return eof;
}
