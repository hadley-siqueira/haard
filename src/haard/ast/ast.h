#ifndef HAARD_AST_H
#define HAARD_AST_H

namespace haard {
    typedef enum AstKind {
        AST_UNKNOWN,
        AST_IMPORT,
        AST_MODULE,
        AST_FUNCTION,
        AST_VARIABLE,
    } AstKind;

    class Ast {
    public:
        Ast();
        virtual ~Ast();

    public:
        void set_kind(AstKind kind);
        AstKind get_kind();

        void set_parent_node(Ast* node);
        Ast* get_parent_node();

    private:
        AstKind kind;
        Ast* parent_node;
    };
}

#endif
