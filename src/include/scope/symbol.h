#ifndef HAARD_SYMBOL_H
#define HAARD_SYMBOL_H

#include <vector>

#include "scope/symbol_descriptor.h"

namespace haard {

    class Symbol {
    public:
        Symbol();
        ~Symbol();

    public:
        void add_descriptor(SymbolDescriptorKind kind, void* descriptor);
        int descriptors_count();
        SymbolDescriptor* get_descriptor(int idx=0);

        //Type* get_type(int idx=0);

    public:
        bool is_function();

    private:
        std::vector<SymbolDescriptor*> descriptors;
    };
}

#endif
