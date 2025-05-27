#ifndef HAARD_AST_H
#define HAARD_AST_H

namespace haard {
    typedef enum AstKind {
        AST_IMPORT
    } AstKind;

    class Ast {
    public:
        Ast();
        virtual ~Ast();

    public:
        void set_kind(AstKind kind);
        AstKind get_kind();

    private:
        AstKind kind;
    };
}

#endif
