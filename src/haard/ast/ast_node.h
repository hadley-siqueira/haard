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
        AST_FUNCTION,
        AST_GENERIC_PARAMETERS,
        AST_FUNCTION_RETURN_TYPE,
        AST_PARAM,
        AST_BLOCK,
        AST_PASS,
        AST_IF,
        AST_ELIF,
        AST_ELSE,
        AST_WHILE,
        AST_RETURN,
        AST_BREAK,
        AST_CONTINUE,
        AST_YIELD,
        AST_GOTO,
        AST_BINDING,
        AST_BINDING_NAME,
        AST_BINDING_TYPE,
        AST_BINDING_EXPRESSION,
        AST_PLUS,
        AST_MINUS,
        AST_TIMES,
        AST_DIVISION,
        AST_ASSIGNMENT,
        AST_PLUS_ASSIGNMENT,
        AST_MINUS_ASSIGNMENT,
        AST_TIMES_ASSIGNMENT,
        AST_DIVISION_ASSIGNMENT,
        AST_INTEGER_DIVISION_ASSIGNMENT,
        AST_MODULO_ASSIGNMENT,
        AST_BITWISE_AND_ASSIGNMENT,
        AST_BITWISE_OR_ASSIGNMENT,
        AST_BITWISE_XOR_ASSIGNMENT,
        AST_BITWISE_NOT_ASSIGNMENT,
        AST_BITWISE_LEFT_SHIFT_ASSIGNMENT,
        AST_BITWISE_RIGHT_SHIFT_ASSIGNMENT,
        AST_BITWISE_UNSIGNED_RIGHT_SHIFT_ASSIGNMENT,
        AST_CAST,
        AST_LOGICAL_OR,
        AST_LOGICAL_AND,
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
        AST_INTEGER_DIVISION,
        AST_MODULO,
        AST_POWER,
        AST_BITWISE_OR,
        AST_BITWISE_XOR,
        AST_BITWISE_AND,
        AST_BITWISE_LEFT_SHIFT,
        AST_BITWISE_RIGHT_SHIFT,
        AST_BITWISE_UNSIGNED_RIGHT_SHIFT,
        AST_LOGICAL_NOT,
        AST_LOGICAL_NOT_OPERATOR,
        AST_ADDRESS_OF,
        AST_DEREFERENCE,
        AST_BITWISE_NOT,
        AST_UNARY_MINUS,
        AST_UNARY_PLUS,
        AST_PRE_INCREMENT,
        AST_PRE_DECREMENT,
        AST_SCOPE,
        AST_DOT,
        AST_ARROW,
        AST_INDEX,
        AST_CALL,
        AST_ARGUMENTS,
        AST_POST_INCREMENT,
        AST_POST_DECREMENT,
        AST_PARENTHESIS,
        AST_IDENTIFIER,
        AST_INTEGER_LITERAL,
        AST_FLOAT_LITERAL,
        AST_STRING_LITERAL,
        AST_CHAR_LITERAL,
        AST_SYMBOL_LITERAL,
        AST_TRUE,
        AST_FALSE,
        AST_NULL_LITERAL,
        AST_THIS,
        AST_LIST,
        AST_ARRAY,
        AST_HASH,
        AST_HASH_PAIR,
        AST_TUPLE,
        AST_CLOSURE,
        AST_CLOSURE_PARAMETER,
        AST_CLOSURE_RETURN_TYPE,
        AST_TEMPLATE_STRING,
        AST_TEMPLATE_STRING_CHUNK,
        AST_INTERPOLATION,
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
