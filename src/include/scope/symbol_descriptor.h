#ifndef HAARD_SYMBL_DESCRIPTOR_H
#define HAARD_SYMBL_DESCRIPTOR_H

namespace haard {
    enum SymbolDescriptorKind {
        SYM_UNKNOWN,
        SYM_CLASS,
        SYM_ENUM,
        SYM_UNION,
        SYM_STRUCT,
        SYM_INTERFACE,
        SYM_FUNCTION,
        SYM_METHOD,

        SYM_PARAMETER,
        SYM_INSTANCE_VARIABLE,
        SYM_LOCAL_VARIABLE,
    };

    class Symbol;
    class SymbolDescriptor {
    public:
        SymbolDescriptor();

    public:
        SymbolDescriptorKind get_kind();
        void set_kind(SymbolDescriptorKind kind);

        void* get_descriptor();
        void set_descriptor(void* descriptor);

    public:
        bool is_named_type();

        Symbol *get_symbol() const;
        void set_symbol(Symbol *newSymbol);

    private:
        SymbolDescriptorKind kind;
        void* descriptor;
        Symbol* symbol;
    };
}

#endif
