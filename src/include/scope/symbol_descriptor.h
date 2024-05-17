#ifndef HAARD_SYMBL_DESCRIPTOR_H
#define HAARD_SYMBL_DESCRIPTOR_H

namespace haard {
    enum SymbolDescriptorKind {
        SYM_UNKNOWN,
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

    class SymbolDescriptor {
    public:
        SymbolDescriptor();

    public:
        SymbolDescriptorKind get_kind();
        void set_kind(SymbolDescriptorKind kind);

        void* get_descriptor();
        void set_descriptor(void* descriptor);

    private:
        SymbolDescriptorKind kind;
        void* descriptor;
    };
}

#endif
