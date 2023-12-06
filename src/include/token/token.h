#ifndef HAARD_TOKEN_H
#define HAARD_TOKEN_H

#include <string>
#include <map>

namespace haard {
    typedef enum TokenKind {
        TK_DEF,
        TK_CLASS,
        TK_VAR,
        TK_CONSTANT,
        TK_STRUCT,
        TK_ENUM,
        TK_UNION,
        TK_DATA,
        TK_THIS,
        TK_IF,
        TK_ELIF,
        TK_ELSE,
        TK_FOR,
        TK_WHILE,
        TK_CONTINUE,
        TK_RETURN,
        TK_GOTO,
        TK_BREAK,
        TK_YIELD,
        TK_FROM,
        TK_AS,
        TK_IMPORT,
        TK_DO,
        TK_IN,
        TK_PASS,
        TK_NEW,
        TK_DELETE,
        TK_CASE,
        TK_DEFAULT,
        TK_WITH,
        TK_SIZEOF,
        TK_AND,
        TK_OR,
        TK_NOT,
        TK_U8,
        TK_U16,
        TK_U32,
        TK_U64,
        TK_I8,
        TK_I16,
        TK_I32,
        TK_I64,
        TK_F32,
        TK_F64,
        TK_VOID,
        TK_SYMBOL,
        TK_BOOL,
        TK_UCHAR,
        TK_CHAR,
        TK_USHORT,
        TK_SHORT,
        TK_UINT,
        TK_INT,
        TK_ULONG,
        TK_LONG,
        TK_FLOAT,
        TK_DOUBLE,
        TK_STR,
        TK_ID,
        TK_TRUE,
        TK_FALSE,
        TK_NULL,
        TK_LITERAL_CHAR,
        TK_LITERAL_INTEGER,
        TK_LITERAL_FLOAT,
        TK_LITERAL_DOUBLE,
        TK_LITERAL_SINGLE_QUOTE_STRING,
        TK_LITERAL_DOUBLE_QUOTE_STRING,
        TK_LITERAL_SYMBOL,
        TK_PLUS,
        TK_MINUS,
        TK_TIMES,
        TK_DIVISION,
        TK_INTEGER_DIVISION,
        TK_MODULO,
        TK_POWER,
        TK_INC,
        TK_DEC,
        TK_BITWISE_AND,
        TK_BITWISE_NOT,
        TK_BITWISE_OR,
        TK_BITWISE_XOR,
        TK_SLL,
        TK_SRL,
        TK_SRA,
        TK_ARROW,
        TK_DOT,
        TK_DOLAR,
        TK_DOUBLE_DOLAR,
        TK_SCOPE,
        TK_INCLUSIVE_RANGE,
        TK_EXCLUSIVE_RANGE,
        TK_EQ,
        TK_NE,
        TK_GT,
        TK_LT,
        TK_GE,
        TK_LE,
        TK_LOGICAL_AND,
        TK_LOGICAL_OR,
        TK_LOGICAL_NOT,
        TK_TERNARY,
        TK_ASSIGNMENT,
        TK_BITWISE_AND_ASSIGNMENT,
        TK_BITWISE_XOR_ASSIGNMENT,
        TK_BITWISE_OR_ASSIGNMENT,
        TK_BITWISE_NOT_ASSIGNMENT,
        TK_DIVISION_ASSIGNMENT,
        TK_INTEGER_DIVISION_ASSIGNMENT,
        TK_MINUS_ASSIGNMENT,
        TK_MODULO_ASSIGNMENT,
        TK_PLUS_ASSIGNMENT,
        TK_TIMES_ASSIGNMENT,
        TK_SLL_ASSIGNMENT,
        TK_SRA_ASSIGNMENT,
        TK_SRL_ASSIGNMENT,
        TK_SPECIAL_ASSIGNMENT,
        TK_HASH_ARROW,
        TK_NEWLINE,
        TK_EOF,
        TK_AT,
        TK_BEGIN,
        TK_END,
        TK_COLON,
        TK_COMMA,
        TK_SEMICOLON,
        TK_LEFT_CURLY_BRACKET,
        TK_LEFT_PARENTHESIS,
        TK_LEFT_SQUARE_BRACKET,
        TK_RIGHT_CURLY_BRACKET,
        TK_RIGHT_PARENTHESIS,
        TK_RIGHT_SQUARE_BRACKET,
        TK_BEGIN_TEMPLATE,
        TK_END_TEMPLATE,
        TK_UNKNOWN,
    } TokenKind;

    const std::map<std::string, int> value_to_token_kind_map = {
        {"def", TK_DEF},
        {"class", TK_CLASS},
        {"var", TK_VAR},
        {"const", TK_CONSTANT},
        {"struct", TK_STRUCT},
        {"enum", TK_ENUM},
        {"union", TK_UNION},
        {"data", TK_DATA},
        {"this", TK_THIS},
        {"if", TK_IF},
        {"elif", TK_ELIF},
        {"else", TK_ELSE},
        {"for", TK_FOR},
        {"while", TK_WHILE},
        {"continue", TK_CONTINUE},
        {"return", TK_RETURN},
        {"goto", TK_GOTO},
        {"break", TK_BREAK},
        {"yield", TK_YIELD},
        {"from", TK_FROM},
        {"as", TK_AS},
        {"import", TK_IMPORT},
        {"do", TK_DO},
        {"in", TK_IN},
        {"pass", TK_PASS},
        {"new", TK_NEW},
        {"delete", TK_DELETE},
        {"case", TK_CASE},
        {"default", TK_DEFAULT},
        {"with", TK_WITH},
        {"sizeof", TK_SIZEOF},
        {"and", TK_AND},
        {"or", TK_OR},
        {"not", TK_NOT},
        {"u8", TK_U8},
        {"u16", TK_U16},
        {"u32", TK_U32},
        {"u64", TK_U64},
        {"i8", TK_I8},
        {"i16", TK_I16},
        {"i32", TK_I32},
        {"i64", TK_I64},
        {"f32", TK_F32},
        {"f64", TK_F64},
        {"void", TK_VOID},
        {"sym", TK_SYMBOL},
        {"bool", TK_BOOL},
        {"uchar", TK_UCHAR},
        {"char", TK_CHAR},
        {"ushort", TK_USHORT},
        {"short", TK_SHORT},
        {"uint", TK_UINT},
        {"int", TK_INT},
        {"ulong", TK_ULONG},
        {"long", TK_LONG},
        {"float", TK_FLOAT},
        {"double", TK_DOUBLE},
        {"str", TK_STR},
        {"id", TK_ID},
        {"true", TK_TRUE},
        {"false", TK_FALSE},
        {"null", TK_NULL},
        {"@lchar", TK_LITERAL_CHAR},
        {"@lint", TK_LITERAL_INTEGER},
        {"@lfloat", TK_LITERAL_FLOAT},
        {"@ldouble", TK_LITERAL_DOUBLE},
        {"@lstr", TK_LITERAL_SINGLE_QUOTE_STRING},
        {"@lsym", TK_LITERAL_SYMBOL},
        {"+", TK_PLUS},
        {"-", TK_MINUS},
        {"*", TK_TIMES},
        {"/", TK_DIVISION},
        {"//", TK_INTEGER_DIVISION},
        {"%", TK_MODULO},
        {"**", TK_POWER},
        {"++", TK_INC},
        {"--", TK_DEC},
        {"&", TK_BITWISE_AND},
        {"~", TK_BITWISE_NOT},
        {"|", TK_BITWISE_OR},
        {"^", TK_BITWISE_XOR},
        {"<<", TK_SLL},
        {">>>", TK_SRL},
        {">>", TK_SRA},
        {"->", TK_ARROW},
        {".", TK_DOT},
        {"$", TK_DOLAR},
        {"$$", TK_DOUBLE_DOLAR},
        {"::", TK_SCOPE},
        {"..", TK_INCLUSIVE_RANGE},
        {"...", TK_EXCLUSIVE_RANGE},
        {"==", TK_EQ},
        {"!=", TK_NE},
        {">", TK_GT},
        {"<", TK_LT},
        {">=", TK_GE},
        {"<=", TK_LE},
        {"&&", TK_LOGICAL_AND},
        {"||", TK_LOGICAL_OR},
        {"!", TK_LOGICAL_NOT},
        {"?", TK_TERNARY},
        {"=", TK_ASSIGNMENT},
        {"&=", TK_BITWISE_AND_ASSIGNMENT},
        {"^=", TK_BITWISE_XOR_ASSIGNMENT},
        {"|=", TK_BITWISE_OR_ASSIGNMENT},
        {"~=", TK_BITWISE_NOT_ASSIGNMENT},
        {"/=", TK_DIVISION_ASSIGNMENT},
        {"//=", TK_INTEGER_DIVISION_ASSIGNMENT},
        {"-=", TK_MINUS_ASSIGNMENT},
        {"%=", TK_MODULO_ASSIGNMENT},
        {"+=", TK_PLUS_ASSIGNMENT},
        {"*=", TK_TIMES_ASSIGNMENT},
        {"<<=", TK_SLL_ASSIGNMENT},
        {">>=", TK_SRA_ASSIGNMENT},
        {">>>=", TK_SRL_ASSIGNMENT},
        {":=", TK_SPECIAL_ASSIGNMENT},
        {"=>", TK_HASH_ARROW},
        {"@@@n", TK_NEWLINE},
        {"@@@eof", TK_EOF},
        {"@", TK_AT},
        {"@begin", TK_BEGIN},
        {"@end", TK_END},
        {":", TK_COLON},
        {",", TK_COMMA},
        {";", TK_SEMICOLON},
        {"{", TK_LEFT_CURLY_BRACKET},
        {"(", TK_LEFT_PARENTHESIS},
        {"[", TK_LEFT_SQUARE_BRACKET},
        {"}", TK_RIGHT_CURLY_BRACKET},
        {")", TK_RIGHT_PARENTHESIS},
        {"]", TK_RIGHT_SQUARE_BRACKET},
        {"@@@tbegin", TK_BEGIN_TEMPLATE},
        {"@@@tend", TK_END_TEMPLATE},
        {"@@@unknown", TK_UNKNOWN},
    };

    const std::map<int, std::string> token_kind_to_str_map = {
        {TK_DEF, "TK_DEF"},
        {TK_CLASS, "TK_CLASS"},
        {TK_VAR, "TK_VAR"},
        {TK_CONSTANT, "TK_CONSTANT"},
        {TK_STRUCT, "TK_STRUCT"},
        {TK_ENUM, "TK_ENUM"},
        {TK_UNION, "TK_UNION"},
        {TK_DATA, "TK_DATA"},
        {TK_THIS, "TK_THIS"},
        {TK_IF, "TK_IF"},
        {TK_ELIF, "TK_ELIF"},
        {TK_ELSE, "TK_ELSE"},
        {TK_FOR, "TK_FOR"},
        {TK_WHILE, "TK_WHILE"},
        {TK_CONTINUE, "TK_CONTINUE"},
        {TK_RETURN, "TK_RETURN"},
        {TK_GOTO, "TK_GOTO"},
        {TK_BREAK, "TK_BREAK"},
        {TK_YIELD, "TK_YIELD"},
        {TK_FROM, "TK_FROM"},
        {TK_AS, "TK_AS"},
        {TK_IMPORT, "TK_IMPORT"},
        {TK_DO, "TK_DO"},
        {TK_IN, "TK_IN"},
        {TK_PASS, "TK_PASS"},
        {TK_NEW, "TK_NEW"},
        {TK_DELETE, "TK_DELETE"},
        {TK_CASE, "TK_CASE"},
        {TK_DEFAULT, "TK_DEFAULT"},
        {TK_WITH, "TK_WITH"},
        {TK_SIZEOF, "TK_SIZEOF"},
        {TK_AND, "TK_AND"},
        {TK_OR, "TK_OR"},
        {TK_NOT, "TK_NOT"},
        {TK_U8, "TK_U8"},
        {TK_U16, "TK_U16"},
        {TK_U32, "TK_U32"},
        {TK_U64, "TK_U64"},
        {TK_I8, "TK_I8"},
        {TK_I16, "TK_I16"},
        {TK_I32, "TK_I32"},
        {TK_I64, "TK_I64"},
        {TK_F32, "TK_F32"},
        {TK_F64, "TK_F64"},
        {TK_VOID, "TK_VOID"},
        {TK_SYMBOL, "TK_SYMBOL"},
        {TK_BOOL, "TK_BOOL"},
        {TK_UCHAR, "TK_UCHAR"},
        {TK_CHAR, "TK_CHAR"},
        {TK_USHORT, "TK_USHORT"},
        {TK_SHORT, "TK_SHORT"},
        {TK_UINT, "TK_UINT"},
        {TK_INT, "TK_INT"},
        {TK_ULONG, "TK_ULONG"},
        {TK_LONG, "TK_LONG"},
        {TK_FLOAT, "TK_FLOAT"},
        {TK_DOUBLE, "TK_DOUBLE"},
        {TK_STR, "TK_STR"},
        {TK_ID, "TK_ID"},
        {TK_TRUE, "TK_TRUE"},
        {TK_FALSE, "TK_FALSE"},
        {TK_NULL, "TK_NULL"},
        {TK_LITERAL_CHAR, "TK_LITERAL_CHAR"},
        {TK_LITERAL_INTEGER, "TK_LITERAL_INTEGER"},
        {TK_LITERAL_FLOAT, "TK_LITERAL_FLOAT"},
        {TK_LITERAL_DOUBLE, "TK_LITERAL_DOUBLE"},
        {TK_LITERAL_SINGLE_QUOTE_STRING, "TK_LITERAL_STRING"},
        {TK_LITERAL_SYMBOL, "TK_LITERAL_SYMBOL"},
        {TK_PLUS, "TK_PLUS"},
        {TK_MINUS, "TK_MINUS"},
        {TK_TIMES, "TK_TIMES"},
        {TK_DIVISION, "TK_DIVISION"},
        {TK_INTEGER_DIVISION, "TK_INTEGER_DIVISION"},
        {TK_MODULO, "TK_MODULO"},
        {TK_POWER, "TK_POWER"},
        {TK_INC, "TK_INC"},
        {TK_DEC, "TK_DEC"},
        {TK_BITWISE_AND, "TK_BITWISE_AND"},
        {TK_BITWISE_NOT, "TK_BITWISE_NOT"},
        {TK_BITWISE_OR, "TK_BITWISE_OR"},
        {TK_BITWISE_XOR, "TK_BITWISE_XOR"},
        {TK_SLL, "TK_SLL"},
        {TK_SRL, "TK_SRL"},
        {TK_SRA, "TK_SRA"},
        {TK_ARROW, "TK_ARROW"},
        {TK_DOT, "TK_DOT"},
        {TK_DOLAR, "TK_DOLAR"},
        {TK_DOUBLE_DOLAR, "TK_DOUBLE_DOLAR"},
        {TK_SCOPE, "TK_SCOPE"},
        {TK_INCLUSIVE_RANGE, "TK_INCLUSIVE_RANGE"},
        {TK_EXCLUSIVE_RANGE, "TK_EXCLUSIVE_RANGE"},
        {TK_EQ, "TK_EQ"},
        {TK_NE, "TK_NE"},
        {TK_GT, "TK_GT"},
        {TK_LT, "TK_LT"},
        {TK_GE, "TK_GE"},
        {TK_LE, "TK_LE"},
        {TK_LOGICAL_AND, "TK_LOGICAL_AND"},
        {TK_LOGICAL_OR, "TK_LOGICAL_OR"},
        {TK_LOGICAL_NOT, "TK_LOGICAL_NOT"},
        {TK_TERNARY, "TK_TERNARY"},
        {TK_ASSIGNMENT, "TK_ASSIGNMENT"},
        {TK_BITWISE_AND_ASSIGNMENT, "TK_BITWISE_AND_ASSIGNMENT"},
        {TK_BITWISE_XOR_ASSIGNMENT, "TK_BITWISE_XOR_ASSIGNMENT"},
        {TK_BITWISE_OR_ASSIGNMENT, "TK_BITWISE_OR_ASSIGNMENT"},
        {TK_BITWISE_NOT_ASSIGNMENT, "TK_BITWISE_NOT_ASSIGNMENT"},
        {TK_DIVISION_ASSIGNMENT, "TK_DIVISION_ASSIGNMENT"},
        {TK_INTEGER_DIVISION_ASSIGNMENT, "TK_INTEGER_DIVISION_ASSIGNMENT"},
        {TK_MINUS_ASSIGNMENT, "TK_MINUS_ASSIGNMENT"},
        {TK_MODULO_ASSIGNMENT, "TK_MODULO_ASSIGNMENT"},
        {TK_PLUS_ASSIGNMENT, "TK_PLUS_ASSIGNMENT"},
        {TK_TIMES_ASSIGNMENT, "TK_TIMES_ASSIGNMENT"},
        {TK_SLL_ASSIGNMENT, "TK_SLL_ASSIGNMENT"},
        {TK_SRA_ASSIGNMENT, "TK_SRA_ASSIGNMENT"},
        {TK_SRL_ASSIGNMENT, "TK_SRL_ASSIGNMENT"},
        {TK_SPECIAL_ASSIGNMENT, "TK_SPECIAL_ASSIGNMENT"},
        {TK_HASH_ARROW, "TK_HASH_ARROW"},
        {TK_NEWLINE, "TK_NEWLINE"},
        {TK_EOF, "TK_EOF"},
        {TK_AT, "TK_AT"},
        {TK_BEGIN, "TK_BEGIN"},
        {TK_END, "TK_END"},
        {TK_COLON, "TK_COLON"},
        {TK_COMMA, "TK_COMMA"},
        {TK_SEMICOLON, "TK_SEMICOLON"},
        {TK_LEFT_CURLY_BRACKET, "TK_LEFT_CURLY_BRACKET"},
        {TK_LEFT_PARENTHESIS, "TK_LEFT_PARENTHESIS"},
        {TK_LEFT_SQUARE_BRACKET, "TK_LEFT_SQUARE_BRACKET"},
        {TK_RIGHT_CURLY_BRACKET, "TK_RIGHT_CURLY_BRACKET"},
        {TK_RIGHT_PARENTHESIS, "TK_RIGHT_PARENTHESIS"},
        {TK_RIGHT_SQUARE_BRACKET, "TK_RIGHT_SQUARE_BRACKET"},
        {TK_BEGIN_TEMPLATE, "TK_BEGIN_TEMPLATE"},
        {TK_END_TEMPLATE, "TK_END_TEMPLATE"},
        {TK_UNKNOWN, "TK_UNKNOWN"},
    };

    class Token {
    public:
        int get_kind() const;
        void set_kind(int newKind);

        int get_line() const;
        void set_line(int newLine);

        int get_column() const;
        void set_column(int newColumn);

        int get_whitespace() const;
        void set_whitespace(int newWhitespace);

        const char* get_value() const;
        void set_value(const char* newValue);

        std::string to_str();

    private:
        int kind;
        int line;
        int column;
        int whitespace;
        const char* value;
    };
}

#endif
