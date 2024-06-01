#ifndef HAARD_SEMANTIC_PASS_H
#define HAARD_SEMANTIC_PASS_H

#include <stack>

#include "scope/scope.h"
#include "log/logger.h"
#include "semantic/semantic_context.h"

namespace haard {
    class SemanticPass {
    public:
        SemanticPass();

    public:
        void enter_scope(Scope* scope, bool set_parent=true);
        void leave_scope();

        Symbol* resolve(std::string name);
        void define(SymbolDescriptorKind kind, std::string name, void* descriptor);

        Logger* get_logger() const;
        void set_logger(Logger* newLogger);

        SemanticContext* get_context() const;
        void set_context(SemanticContext* newContext);

    protected:
        SemanticContext* context;
        Logger* logger;
    };
}

#endif
