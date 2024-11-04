#ifndef HAARD_SYMBOL_H
#define HAARD_SYMBOL_H

#include <vector>

#include "haard/ast/ast.h"

namespace haard {
    typedef enum SymbolKind {
        SYM_CLASS,
        SYM_STRUCT,
        SYM_UNION,
        SYM_ENUM
    } SymbolKind;

    class Symbol {
    public:
        Symbol();
        Symbol(SymbolKind kind, const std::string& name, Ast* node);

    public:
        Ast* get_ast() const;
        void set_ast(Ast* ast);

        const std::string& get_name() const;
        void set_name(const std::string& name);

        SymbolKind get_kind() const;
        void set_kind(SymbolKind kind);

    private:
        SymbolKind kind;
        std::string name;
        Ast* ast;
    };
}

#endif
