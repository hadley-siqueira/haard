#ifndef HAARD_MODULE_H
#define HAARD_MODULE_H

#include <haard/log/log.h>
#include <haard/token_stream/token_stream.h>
#include <haard/ast/ast.h>
#include <haard/module_finder/module_finder.h>
#include <haard/source_file/source_file.h>
#include <haard/string_table/string_table.h>
#include <haard/symbol_table/symbol_table.h>
#include <haard/type_table/type_table.h>
#include <haard/resolution_table/resolution_table.h>
#include <string>
#include <vector>

namespace haard {
    // One import, resolved. The alias is interned in the **importer's** string
    // table and is INVALID_STRING when the import was written without an 'as'.
    //
    // A star import gives its alias to every file it expanded to (record
    // 0006): the alias names the pool and not one module, so 'io::foo' is as
    // ambiguous as a bare 'foo' and resolves the same way
    struct Dependency {
        u32 module;
        u32 alias;
    };

    class Module {
        public:
            Module();

        public:
            TokenStream* get_tokens();
            SourceFile* get_source_file();
            Ast* get_ast();
            Logger* get_logger();

            // the two tables record 0012 makes per module: the names this file
            // uses, each stored once, and the symbols that index into them
            StringTable* get_strings();
            SymbolTable* get_symbols();
            TypeTable* get_types();

            // what each node turned out to be, per record 0019: the answers
            // the type phase worked out, kept instead of thrown away
            ResolutionTable* get_resolutions();

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

            // the modules this one imports, in the order its source wrote
            // the imports, with a star import's expansion sorted in place by
            // the finder. The order is not an incidental listing: record 0009
            // resolves a bare name to the first import that brings it, so
            // this vector is what decides which declaration a name means.
            //
            // The same module may appear twice -- 'import std.io.*' next to
            // 'import std.io.tcp' is one declaration arriving by two routes,
            // which record 0009 calls first-wins choosing between equals, so
            // there is nothing to deduplicate.
            //
            // It is also the dep table record 0008 keeps in the blob, and the
            // graph record 0015 walks to decide what a rebuild has to reach
            void add_dependency(u32 module, u32 alias);
            const std::vector<Dependency>& get_dependencies();

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
            std::vector<Dependency> dependencies;
            bool parsed;

            SourceFile source_file;
            TokenStream tokens;
            Ast ast;
            Logger logger;
            StringTable strings;
            SymbolTable symbols;
            TypeTable types;
            ResolutionTable resolutions;
    };
}

#endif
