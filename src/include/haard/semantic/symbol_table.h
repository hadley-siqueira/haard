#ifndef HAARD_SYMBOL_TABLE_H
#define HAARD_SYMBOL_TABLE_H

namespace haard {
    class SymbolTable {
    public:
        SymbolTable();
        ~SymbolTable();

    public:
        SymbolTable* get_parent() const;
        void set_parent(SymbolTable* parent);

    private:
        SymbolTable* parent;
    };
}

#endif
