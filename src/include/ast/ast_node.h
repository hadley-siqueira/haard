#ifndef HAARD_AST_NODE_H
#define HAARD_AST_NODE_H

#include "token/token.h"

namespace haard {
    typedef enum AstKind {
        AST_MODULE,
        AST_FUNCTION,
        AST_CLASS,
        AST_STRUCT,
        AST_ENUM,
        AST_UNION,
        AST_IMPORT,
        AST_ID,

        AST_UNKNOWN_STATEMENT,
        AST_COMPOUND_STATEMENT,
        AST_EXPRESSION_STATEMENT,
        STMT_WHILE,
        STMT_FOR,
        STMT_RETURN,

        AST_EXPRESSION_UNKNOWN,

        // Binary operators
        EXPR_ASSIGNMENT,
        EXPR_BITWISE_AND_ASSIGNMENT,
        EXPR_BITWISE_XOR_ASSIGNMENT,
        EXPR_BITWISE_OR_ASSIGNMENT,
        EXPR_BITWISE_NOT_ASSIGNMENT,
        EXPR_DIVISION_ASSIGNMENT,
        EXPR_INTEGER_DIVISION_ASSIGNMENT,
        EXPR_MINUS_ASSIGNMENT,
        EXPR_MODULO_ASSIGNMENT,
        EXPR_PLUS_ASSIGNMENT,
        EXPR_TIMES_ASSIGNMENT,
        EXPR_SHIFT_LEFT_LOGICAL_ASSIGNMENT,
        EXPR_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT,
        EXPR_SHIFT_RIGHT_LOGICAL_ASSIGNMENT,

        EXPR_CAST,

        EXPR_LOGICAL_OR,
        EXPR_LOGICAL_AND,

        EXPR_EQUAL,
        EXPR_NOT_EQUAL,

        EXPR_LESS_THAN,
        EXPR_GREATER_THAN,
        EXPR_LESS_THAN_OR_EQUAL,
        EXPR_GREATER_THAN_OR_EQUAL,

        EXPR_IN,
        EXPR_NOT_IN,

        EXPR_INCLUSIVE_RANGE,
        EXPR_EXCLUSIVE_RANGE,

        EXPR_PLUS,
        EXPR_MINUS,

        EXPR_TIMES,
        EXPR_DIVISION,
        EXPR_MODULO,
        EXPR_INTEGER_DIVISION,

        EXPR_POWER,

        EXPR_BITWISE_OR,
        EXPR_BITWISE_XOR,
        EXPR_BITWISE_AND,

        EXPR_SHIFT_LEFT_LOGICAL,
        EXPR_SHIFT_RIGHT_ARITHMETIC,
        EXPR_SHIFT_RIGHT_LOGICAL,

        EXPR_DOT,
        EXPR_ARROW,
        EXPR_INDEX,
        EXPR_CALL,
        EXPR_HASH_PAIR,

        EXPR_ADDRESS_OF,
        EXPR_DEREFERENCE,
        EXPR_UNARY_PLUS,
        EXPR_UNARY_MINUS,
        EXPR_LOGICAL_NOT,
        EXPR_BITWISE_NOT,
        EXPR_SIZEOF,
        EXPR_NEW,
        EXPR_DELETE,
        EXPR_DELETE_ARRAY,
        EXPR_PARENTHESIS,

        EXPR_PRE_INCREMENT,
        EXPR_PRE_DECREMENT,
        EXPR_POS_INCREMENT,
        EXPR_POS_DECREMENT,

        // Literals
        EXPR_LITERAL_BOOLEAN,
        EXPR_LITERAL_INTEGER,
        EXPR_LITERAL_FLOAT,
        EXPR_LITERAL_DOUBLE,
        EXPR_LITERAL_CHAR,
        EXPR_LITERAL_STRING,
        EXPR_LITERAL_LIST,
        EXPR_LITERAL_ARRAY,
        EXPR_LITERAL_HASH,
        EXPR_LITERAL_TUPLE,

        EXPR_SEQUENCE,

        // Types
        TYPE_BOOL,
        TYPE_CHAR,
        TYPE_UCHAR,
        TYPE_SHORT,
        TYPE_USHORT,
        TYPE_INT,
        TYPE_UINT,
        TYPE_LONG,
        TYPE_ULONG,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_I8,
        TYPE_U8,
        TYPE_I16,
        TYPE_U16,
        TYPE_I32,
        TYPE_U32,
        TYPE_I64,
        TYPE_U64,
        TYPE_F32,
        TYPE_F64,
        TYPE_SYMBOL,
        TYPE_VOID,
        TYPE_UNKNOWN,
        TYPE_POINTER,
        TYPE_REFERENCE,
        TYPE_LIST,
        TYPE_ARRAY,
        TYPE_STR,
        TYPE_NAMED,
        TYPE_TUPLE,
        TYPE_FUNCTION,
        AST_GENERICS
    } AstKind;

    class AstNode {
    public:
        virtual ~AstNode();

    public:
        int get_kind() const;
        void set_kind(int newKind);

        const Token& get_token() const;
        void set_token(const Token& newToken);

    private:
        int kind;
        Token token;
    };
}

#endif
