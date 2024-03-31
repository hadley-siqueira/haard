#ifndef HAARD_SEMANTIC_PASS_H
#define HAARD_SEMANTIC_PASS_H

#include <stack>

#include "symbol_table/symbol_table.h"
#include "log/logger.h"

namespace haard {
    class SemanticPass {
    public:
        void enter_scope(SymbolTable* scope);
        void leave_scope();

        Symbol* resolve(std::string name);

        Logger* get_logger() const;
        void set_logger(Logger* newLogger);

    protected:
        SymbolTable* current_scope;
        std::stack<SymbolTable*> scope_stack;
        Logger* logger;
    };
}

#endif
