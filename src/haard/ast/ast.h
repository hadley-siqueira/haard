#ifndef HAARD_AST_H
#define HAARD_AST_H

namespace haard {
    typedef enum AstKind {
        AST_UNKNOWN,
        AST_MODULES,
        AST_MODULE,
        AST_IMPORT,
        AST_FUNCTION,
        AST_VARIABLE,

        AST_TYPE_PRIMITIVE,
        AST_TYPE_INDIRECTION,
        AST_TYPE_REFERENCE,
        AST_TYPE_RVALUE_REFERENCE,

        AST_ASSIGNMENT,

        AST_PLUS,
        AST_MINUS,

        AST_POS_INC,
        AST_POS_DEC,

        AST_LITERAL,
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
