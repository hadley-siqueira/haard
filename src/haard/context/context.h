#ifndef HAARD_CONTEXT_H
#define HAARD_CONTEXT_H

#include <haard/tokens/tokens.h>
#include <haard/source_file/source_file.h>

namespace haard {
    class Context {
        public:
            Tokens* get_tokens();
            SourceFile* get_source_file();

        public:
            void inspect_tokens();

        private:
            SourceFile source_file;
            Tokens tokens;
    };
}

#endif
