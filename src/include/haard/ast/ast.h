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
        AST_IMPORT_PATH,
        AST_IMPORT_PATH_MEMBER,
        AST_IMPORT_ALIAS,

        AST_GENERIC_APPLICATION,
        AST_SCOPE,
        AST_ID,

        AST_UNKNOWN_STATEMENT,
        AST_COMPOUND_STATEMENT,
        AST_EXPRESSION,
        AST_EXPRESSION_WITH_SEMICOLON,
        AST_WHILE,

        AST_FOR,
        AST_FOR_RANGE,
        AST_FOR_INIT,
        AST_FOR_TEST,
        AST_FOR_UPDATE,

        AST_IF,
        AST_ELIF,
        AST_ELSE,
        AST_RETURN,

        AST_EXPRESSION_UNKNOWN,

        // Binary operators
        AST_ASSIGNMENT,
        AST_BITWISE_AND_ASSIGNMENT,
        AST_BITWISE_XOR_ASSIGNMENT,
        AST_BITWISE_OR_ASSIGNMENT,
        AST_BITWISE_NOT_ASSIGNMENT,
        AST_DIVISION_ASSIGNMENT,
        AST_INTEGER_DIVISION_ASSIGNMENT,
        AST_MINUS_ASSIGNMENT,
        AST_MODULO_ASSIGNMENT,
        AST_PLUS_ASSIGNMENT,
        AST_TIMES_ASSIGNMENT,
        AST_SHIFT_LEFT_LOGICAL_ASSIGNMENT,
        AST_SHIFT_RIGHT_ARITHMETIC_ASSIGNMENT,
        AST_SHIFT_RIGHT_LOGICAL_ASSIGNMENT,

        AST_CAST,

        AST_LOGICAL_OR,
        AST_OR,
        AST_LOGICAL_AND,
        AST_AND,

        AST_EQUAL,
        AST_NOT_EQUAL,

        AST_LESS_THAN,
        AST_GREATER_THAN,
        AST_LESS_THAN_OR_EQUAL,
        AST_GREATER_THAN_OR_EQUAL,

        AST_IN,
        AST_NOT_IN,

        AST_INCLUSIVE_RANGE,
        AST_EXCLUSIVE_RANGE,

        AST_PLUS,
        AST_MINUS,

        AST_TIMES,
        AST_DIVISION,
        AST_MODULO,
        AST_INTEGER_DIVISION,

        AST_POWER,

        AST_BITWISE_OR,
        AST_BITWISE_XOR,
        AST_BITWISE_AND,

        AST_SHIFT_LEFT_LOGICAL,
        AST_SHIFT_RIGHT_ARITHMETIC,
        AST_SHIFT_RIGHT_LOGICAL,

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
        AST_SIZEOF,
        AST_NEW,
        AST_DELETE,
        AST_DELETE_ARRAY,
        AST_PARENTHESIS,
        AST_DOUBLE_DOLAR,

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
        void add_child(AstKind kind, Token& token);
        void add_child(AstKind kind, Ast* subchild);
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
