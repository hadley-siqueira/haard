#ifndef HAARD_SCANNER_H
#define HAARD_SCANNER_H

#include <haard/context/context.h>

namespace haard {
    class Scanner {
        public:
            void get_tokens(Context& context);

        private:
            std::vector<Token>& tokens;
            SourceFile& source_file;
    };
}

#endif
