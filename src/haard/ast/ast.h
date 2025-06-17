#ifndef HAARD_AST_H
#define HAARD_AST_H

namespace haard {
    typedef enum AstKind {
        AST_UNKNOWN,
        AST_IMPORT,
        AST_MODULE,
        AST_FUNCTION,
        AST_VARIABLE,

        AST_TYPE_U8,
        AST_TYPE_I8,
        AST_TYPE_U16,
        AST_TYPE_I16,
        AST_TYPE_U32,
        AST_TYPE_I32,
        AST_TYPE_U64,
        AST_TYPE_I64,
        AST_TYPE_F32,
        AST_TYPE_F64,
        AST_TYPE_POINTER,
        AST_TYPE_REFERENCE,
        AST_TYPE_RVALUE_REFERENCE,

        AST_LITERAL_CHAR,
        AST_LITERAL_INTEGER,
        AST_LITERAL_REAL,
        AST_LITERAL_SYMBOL,
        AST_LITERAL_STRING,
        AST_LITERAL_BOOL,
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
