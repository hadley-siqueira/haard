#ifndef HAARD_SYMBOL_H
#define HAARD_SYMBOL_H

#include <string>

namespace haard {
    typedef enum SymbolKind {
        SYM_UNKNOWN,
        SYM_FUNCTION,
        SYM_METHOD,
        SYM_FIELD,
        SYM_VARIABLE,
        SYM_CLASS,
        SYM_STRUCT,
        SYM_ENUM,
        SYM_UNION,
    } SymbolKind;

    class SymbolTable;
    class AstNode;

    class Symbol {
    public:
        Symbol();
        Symbol(SymbolKind kind, AstNode* descriptor=nullptr);

    public:
        SymbolKind get_kind() const;
        void set_kind(SymbolKind kind);

        AstNode* get_descriptor() const;
        void set_descriptor(AstNode* descriptor);

        SymbolTable* get_symbol_table() const;
        void set_symbol_table(SymbolTable* symbol_table);

        std::string get_name();

    private:
        SymbolKind kind;
        SymbolTable* symbol_table;
        AstNode* descriptor;
    };
}

#endif
