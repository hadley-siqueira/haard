#include <haard/tokens/tokens.h>

using namespace haard;

Tokens::Tokens() {
    eof.set_kind(TK_EOF);
}

void Tokens::reset() {
    tokens.clear();
}

void Tokens::push(Token& token) {
    tokens.push_back(token);
}

std::vector<Token>& Tokens::get_tokens() {
    return tokens;
}

Token& Tokens::get_token(u32 index) {
    if (index < tokens.size()) {
        return tokens[index];
    }

    return eof;
}
