#ifndef HAARD_CONTEXT_H
#define HAARD_CONTEXT_H

#include <haard/log/log.h>
#include <haard/token_stream/token_stream.h>
#include <haard/ast/ast.h>
#include <haard/source_file/source_file.h>

namespace haard {
    class Context {
        public:
            TokenStream* get_tokens();
            SourceFile* get_source_file();
            Ast* get_ast();
            Logger* get_logger();

            std::string_view get_token_value(u32 token);

        public:
            void inspect_tokens();
            void inspect_ast();

        private:
            SourceFile source_file;
            TokenStream tokens;
            Ast ast;
            Logger logger;
    };
}

#endif
