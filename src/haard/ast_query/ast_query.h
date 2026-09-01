#ifndef HAARD_AST_QUERY_H
#define HAARD_AST_QUERY_H

#include <haard/module/module.h>
#include <string>
#include <vector>

namespace haard {
    // Read-only questions about what a module declares. The third of the
    // family: Ast holds the nodes, AstBuilder writes them, this one reads them
    // back.
    //
    // It takes a Module and not an Ast because half of these questions cannot
    // be answered by the tree alone: the segments of 'import a.b.c' are token
    // indices, and turning them into text needs the token stream and the
    // source file. Only a Module has all three.
    //
    // Every answer here assumes the tree parsed clean, which the Driver's
    // phase gate guarantees: a statement that failed logged an error and the
    // compilation stopped before anything got to ask.
    class AstQuery {
        public:
            AstQuery();

        public:
            void set_module(Module* module);

        public:
            std::vector<u32> get_imports();
            std::vector<u32> get_functions();
            std::vector<u32> get_classes();
            std::vector<u32> get_structs();
            std::vector<u32> get_enums();
            std::vector<u32> get_unions();

            // the 'let' and 'const' statements of the module. One statement
            // may declare several names, so a caller after the names walks the
            // AST_BINDING children of each
            std::vector<u32> get_global_variables();

        public:
            // 'import a.b.c as x' gives back "a.b.c". The '*' of a star import
            // is not part of the name: ask is_star_import for that
            std::string get_import_name(u32 import);
            bool is_star_import(u32 import);

            // empty when the import has no 'as'
            std::string get_import_alias(u32 import);

            // the name a 'def', a 'class', a 'struct', an 'enum' or a 'union'
            // declares. Empty for a 'let' or a 'const', which declare one name
            // per AST_BINDING child instead of one for the statement
            std::string get_declaration_name(u32 declaration);

        private:
            // A declaration is a child of the root and never deeper. Walking
            // the node vector instead would be both wrong and slower: a method
            // is an AST_FUNCTION too and a local is an AST_LET_DECLARATION
            // too, and the vector holds every literal and every operator of
            // the file next to the few dozen declarations being looked for
            std::vector<u32> collect(AstNodeKind kind);
            std::vector<u32> collect(AstNodeKind first, AstNodeKind second);

            u32 find_child(u32 parent, AstNodeKind kind);
            std::string text_of(u32 node);

        private:
            Module* module;
            Ast* ast;
    };
}

#endif
