#ifndef HAARD_AST_H
#define HAARD_AST_H

#include <string>
#include <vector>
#include <haard/token/token.h>

namespace haard{
    typedef enum AstKind {
        AST_MODULE,
        AST_IMPORT,
        AST_IMPORT_PATH,
        AST_IMPORT_ALIAS,
        AST_DEF,
        AST_DEF_NAME,
        AST_TYPE_PARAMETERS,
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
            std::string get_kind_as_string();
            Token& get_token();

            virtual std::string to_json();
            virtual void add_child(Ast* child);
            virtual void add_child_as(AstKind kind, Ast* child);
            virtual std::vector<Ast*> get_children();
            virtual std::vector<Ast*> get_children(AstKind kind);
            virtual Ast* get_child(AstKind kind);

        protected:
            AstKind kind;
            Token token;
    };
}

#endif 
