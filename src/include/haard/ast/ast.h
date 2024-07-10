#ifndef HAARD_AST_H
#define HAARD_AST_H

#include <vector>
#include <string>

#include "haard/token/token.h"

namespace haard::ast {
    typedef enum AstKind {
        AST_UNKNOWN,

        AST_MODULE,

        AST_FUNCTION,
        AST_RETURN_TYPE,
        AST_PARAMETERS,
        AST_FUNCTION_STATEMENTS,
        AST_PARAMETER,

        AST_CLASS,
        AST_STRUCT,
        AST_ENUM,
        AST_UNION,

        AST_IMPORT,
        AST_PATH,
        AST_ALIAS,

        AST_GENERIC_APPLICATION,
        AST_SCOPE,
        AST_ID,

        AST_UNKNOWN_STATEMENT,
        AST_COMPOUND_STATEMENT,
        AST_EXPRESSION,
        STMT_WHILE,
        STMT_FOR,
        STMT_IF,
        STMT_ELIF,
        STMT_ELSE,
        AST_RETURN,

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

        AST_DOT,
        AST_ARROW,
        AST_INDEX,
        AST_CALL,
        AST_ARGUMENTS,
        AST_NAMED_ARGUMENT,
        AST_HASH_PAIR,

        AST_ADDRESS_OF,
        AST_DEREFERENCE,
        AST_UNARY_PLUS,
        AST_UNARY_MINUS,
        AST_LOGICAL_NOT,
        AST_NOT,
        AST_BITWISE_NOT,
        EXPR_SIZEOF,
        EXPR_NEW,
        EXPR_DELETE,
        EXPR_DELETE_ARRAY,
        EXPR_PARENTHESIS,
        EXPR_DOUBLE_DOLAR,

        AST_PRE_INCREMENT,
        AST_PRE_DECREMENT,
        AST_POS_INCREMENT,
        AST_POS_DECREMENT,

        // Literals
        AST_THIS,
        AST_NULL,
        AST_LITERAL_BOOLEAN,
        AST_LITERAL_INTEGER,
        AST_LITERAL_FLOAT,
        AST_LITERAL_DOUBLE,
        AST_LITERAL_CHAR,
        AST_LITERAL_STRING,
        AST_LIST,
        AST_ARRAY,
        AST_HASH,
        AST_TUPLE,

        AST_SEQUENCE,

        // Types
        AST_TYPE_BOOL,
        AST_TYPE_CHAR,
        AST_TYPE_UCHAR,
        AST_TYPE_SHORT,
        AST_TYPE_USHORT,
        AST_TYPE_INT,
        AST_TYPE_UINT,
        AST_TYPE_LONG,
        AST_TYPE_ULONG,
        AST_TYPE_FLOAT,
        AST_TYPE_DOUBLE,
        AST_TYPE_I8,
        AST_TYPE_U8,
        AST_TYPE_I16,
        AST_TYPE_U16,
        AST_TYPE_I32,
        AST_TYPE_U32,
        AST_TYPE_I64,
        AST_TYPE_U64,
        AST_TYPE_F32,
        AST_TYPE_F64,
        AST_TYPE_SYMBOL,
        AST_TYPE_VOID,
        AST_TYPE_UNKNOWN,
        AST_TYPE_POINTER,
        AST_TYPE_REFERENCE,
        AST_TYPE_LIST,
        AST_TYPE_ARRAY,
        AST_TYPE_STR,
        AST_TYPE_NAMED,
        AST_TYPE_TUPLE,
        AST_TYPE_FUNCTION,
        AST_GENERICS
    } AstKind;

    class Ast {
    public:
        Ast();
        Ast(AstKind type);
        Ast(AstKind type, Token& token);
        virtual ~Ast();

    public:
        AstKind get_kind() const;
        int get_line() const;
        int get_column() const;
        const std::string& get_value() const;

        void set_kind(AstKind kind);
        void set_line(int line);
        void set_column(int column);
        void set_value(const std::string& value);

        void add_child(Ast* child);
        Ast* get_child(size_t index=0);
        Ast* get_child(AstKind type);
        size_t children_count();

        void set_from_token(Token& token);

    private:
        AstKind kind;
        int line;
        int column;
        std::string value;
        std::vector<Ast*> children;
    };
}

#endif
