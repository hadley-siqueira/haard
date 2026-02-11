#ifndef HAARD_CONTEXT_H
#define HAARD_CONTEXT_H

#include <haard/token/token.h>
#include <haard/source_file/source_file.h>
#include <vector>

namespace haard {
    class Context {
        public:
            std::vector<Token>& get_tokens();
            SourceFile& get_source_file();

            void reset_tokens();

        private:
            SourceFile source_file;
            std::vector<Token> tokens;
    };
}

#endif
