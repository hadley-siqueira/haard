#ifndef HAARD_SEMANTIC_PASS_H
#define HAARD_SEMANTIC_PASS_H

#include <stack>

#include "symbol_table/symbol_table.h"
#include "log/logger.h"

namespace haard {
    class SemanticPass {
    public:
        SemanticPass();

    public:
        void enter_scope(Scope* scope, bool set_parent=true);
        void leave_scope();

        Symbol* resolve(std::string name);
        void define(SymbolKind kind, std::string name, void* descriptor);

        Logger* get_logger() const;
        void set_logger(Logger* newLogger);

    protected:
        Scope* current_scope;
        std::stack<Scope*> scope_stack;
        Logger* logger;
    };
}

#endif
