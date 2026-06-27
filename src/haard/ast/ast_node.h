#ifndef HAARD_AST_NODE_H
#define HAARD_AST_NODE_H

#include <haard/haard.h>

namespace haard {
    typedef enum AstNodeKind {
        AST_UNKNOWN,
        AST_MODULE,
        AST_IMPORT,
        AST_IMPORT_PATH,
        AST_IMPORT_PATH_SEGMENT,
        AST_IMPORT_ALIAS,
        AST_LET_DECLARATION,
        AST_CONST_DECLARATION,
        AST_PARAM,
        AST_BINDING,
        AST_BINDING_NAME,
        AST_BINDING_TYPE,
        AST_BINDING_EXPRESSION,
        AST_IDENTIFIER,
    } AstNodeKind;

    class AstNode {
        public:
            AstNode();

        public:
            void set_kind(AstNodeKind kind);
            AstNodeKind get_kind();

            void set_token(u32 token);
            u32 get_token();

            void set_sibling(u32 sibling);
            u32 get_sibling();

            void set_children(u32 child);
            u32 get_children();

        private:
            u8 kind;
            u32 token;
            u32 sibling;
            u32 children;
    };
}

#endif
