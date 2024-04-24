#ifndef HAARD_SYMBOL_H
#define HAARD_SYMBOL_H

#include <vector>

#include "ast/function.h"

namespace haard {
    enum SymbolKind {
        SYM_CLASS,
        SYM_ENUM,
        SYM_UNION,
        SYM_STRUCT,
        SYM_FUNCTION,
        SYM_METHOD,

        SYM_PARAMETER,
        SYM_INSTANCE_VARIABLE,
        SYM_LOCAL_VARIABLE,
    };

    class Symbol {
    public:
        Symbol();

    public:
        SymbolKind get_kind() const;
        void set_kind(SymbolKind newKind);

        void add_descriptor(void* descriptor);
        int descriptors_count();
        void* get_descriptor(int idx);

    public:
        bool is_function();

    private:
        SymbolKind kind;
        std::vector<void*> descriptors;
    };
}

#endif
