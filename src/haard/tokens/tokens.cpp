#include <haard/tokens/tokens.h>

using namespace haard;

void Tokens::reset() {
    tokens.clear();
}

void Tokens::push(Token& token) {
    tokens.push_back(token);
}

std::vector<Token>& Tokens::get_tokens() {
    return tokens;
}
