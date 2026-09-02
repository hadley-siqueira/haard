#ifndef HAARD_MODULE_H
#define HAARD_MODULE_H

#include <haard/log/log.h>
#include <haard/token_stream/token_stream.h>
#include <haard/ast/ast.h>
#include <haard/module_finder/module_finder.h>
#include <haard/source_file/source_file.h>
#include <string>

namespace haard {
    class Module {
        public:
            Module();

        public:
            TokenStream* get_tokens();
            SourceFile* get_source_file();
            Ast* get_ast();
            Logger* get_logger();

            std::string_view get_token_value(u32 token);

        public:
            // the dotted name this file is known by, 'app.main'. A module is
            // a file, so its name is its path with the separators turned back
            // into dots and its root's name in front
            void set_name(const std::string& name);
            const std::string& get_name();

            // the root this file sits in, which decides what its own imports
            // resolve to. INVALID_ROOT when it was compiled without a table
            void set_root(u32 root);
            u32 get_root();

            // whether the parser finished this module's tree. It is not the
            // same question as 'has no errors': an import that resolved to
            // nothing is logged against a module that parsed perfectly, and
            // its tree is still there to be read
            void set_parsed(bool parsed);
            bool is_parsed();

        public:
            void inspect_tokens();
            void inspect_ast();

        private:
            std::string name;
            u32 root;
            bool parsed;

            SourceFile source_file;
            TokenStream tokens;
            Ast ast;
            Logger logger;
    };
}

#endif
