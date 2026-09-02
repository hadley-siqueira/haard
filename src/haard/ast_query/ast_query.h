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

            // the span a diagnostic about this import underlines: the whole
            // 'a.b.c', from the first segment to the last. Not the 'import'
            // keyword, which is the same six characters in every file and
            // says nothing about which import went wrong
            u32 get_import_offset(u32 import);
            u32 get_import_length(u32 import);

            // every declaration the module makes, in the order the source
            // wrote them: the four type declarations, the functions and the
            // 'let' and 'const' statements, mixed. A caller that needs to
            // know which is which reads the node's kind.
            //
            // The typed getters above answer 'what functions are there'; this
            // one answers 'what does this file declare, in order', which is a
            // different question and the one a phase that walks everything
            // once has to ask
            std::vector<u32> get_declarations();

            // the members of a 'class', a 'struct', an 'enum' or a 'union':
            // the methods and the fields of its body, in source order. Empty
            // for a body that is only 'pass'
            std::vector<u32> get_members(u32 declaration);

            // the parameters of a 'def', in source order. They are children
            // of the function and not of its block: a parameter is written on
            // a line of its own inside the function, and Parser::parse_function
            // reads them before the block opens
            std::vector<u32> get_params(u32 function);

            // the generic parameters of a 'def', a 'class', a 'struct', an
            // 'enum' or a 'union'. These are the one declaration whose name is
            // not wrapped in an AST_BINDING_NAME: the nodes given back are the
            // identifiers themselves, which is both the name and what a symbol
            // points at
            std::vector<u32> get_generic_parameters(u32 declaration);

            // the type node a declaration writes, 0 when it writes none. A
            // field, a parameter and a binding hang it on an AST_BINDING_TYPE
            // and a function on an AST_FUNCTION_RETURN_TYPE, and both wrap the
            // type rather than being it
            u32 get_written_type(u32 declaration);

            // the type a 'class', a 'struct' or a 'union' derives from, 0
            // when it derives from nothing. The grammar reads one type between
            // brackets and Hadley settled single inheritance on 2026-09-02, so
            // there is one of these and never a list
            u32 get_super_type(u32 declaration);

            // the expression a declaration was given, 0 when it was given
            // none, and what inference reads. A binding has one, and so does
            // an assignment that declares by being written -- 'let' is not
            // required, so 'n = 1' with no n in view declares n and its right
            // side is what n was given
            u32 get_binding_expression(u32 declaration);

            // the names a 'for ... in' binds, as the identifiers themselves.
            //
            // They are not one node: the head of a for-each is a list of comma
            // separated expressions whose last one is the 'in', so
            // 'for key, value in pairs' writes 'key' as a sibling of the 'in'
            // and 'value' as its left side. Anything in that position which is
            // not an identifier binds nothing and is left out
            std::vector<u32> get_loop_variables(u32 for_each);

            // the block a 'def' or a closure runs, 0 when it has none
            u32 get_block(u32 node);

            // the children of a node, in source order. The walk inside a
            // function is not a walk of known statement kinds: an 'if', a
            // 'while' and a 'for' all reach their block the same way, so a
            // phase that only cares about blocks and closures recurses instead
            // of learning every statement there is
            std::vector<u32> get_children(u32 node);

            // the AST_BINDING a 'let' or a 'const' hangs its name, its type
            // and its value on. It is what a symbol points at and the
            // identifier is not, because an AstNode carries children and
            // siblings and no parent: from the name alone there is no way
            // back up to the type
            u32 get_binding(u32 statement);

            // the names that binding declares, in source order. One, or
            // several when a tuple target takes a value apart:
            // 'let (a, b) = pair'
            std::vector<std::string> get_binding_names(u32 statement);

            // the name a 'def', a 'class', a 'struct', an 'enum' or a 'union'
            // declares, and the name an assignment declares by being written.
            // Empty for a 'let' or a 'const', which declare one name per
            // AST_BINDING child instead of one for the statement
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

            // the first or the last AST_IMPORT_PATH_SEGMENT of an import
            u32 import_segment(u32 import, bool last);
            std::string text_of(u32 node);

        private:
            Module* module;
            Ast* ast;
    };
}

#endif
