#ifndef HAARD_TOKEN_STREAM_H
#define HAARD_TOKEN_STREAM_H

#include <vector>
#include <haard/token/token.h>

namespace haard {
    class TokenStream {
        public:
            TokenStream();

        public:
            void reset();
            void push(Token& token);
            size_t size();
            std::vector<Token>& get_tokens();
            Token& get_token(u32 index);

        private:
            std::vector<Token> tokens;
            Token eof;
    };
}

#endif
