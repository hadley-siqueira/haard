#ifndef HAARD_AST_H
#define HAARD_AST_H

#include <haard/token/token.h>

namespace haard{
    typedef enum AstKind {
        AST_MODULE,
        AST_IMPORT,
        AST_IMPORT_PATH,
        AST_DEF,
        AST_CLASS,
        AST_ENUM,
        AST_UNION,
        AST_STRUCT,
        AST_BINOP,
        AST_IDENTIFIER,
        AST_UNKNOWN,
    } AstKind;

    class Ast {
        public:
            Ast();
            Ast(AstKind kind);
            virtual ~Ast();

        public:
            void set_kind(AstKind kind);
            void set_token(Token& token);

            AstKind get_kind();
            Token get_token();

        private:
            AstKind kind;
            Token token;
    };
}

#endif 
