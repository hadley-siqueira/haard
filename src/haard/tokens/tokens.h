#ifndef HAARD_TOKENS_H
#define HAARD_TOKENS_H

#include <vector>
#include <haard/token/token.h>

namespace haard {
    class Tokens {
        public:
            void reset();
            void push(Token& token);
            std::vector<Token>& get_tokens();

        private:
            std::vector<Token> tokens;
    };
}

#endif
