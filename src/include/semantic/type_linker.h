#ifndef HAARD_SEMANTIC_TYPE_LINKER_H
#define HAARD_SEMANTIC_TYPE_LINKER_H

#include "ast/types/types.h"
#include "scope/scope.h"
#include "log/logger.h"
#include "semantic/semantic_context.h"

namespace haard {
    class TypeLinker {
    public:
        void build(Type* type);
        void build_pointer_type(PointerType* type);
        void build_named_type(NamedType* type);
        Logger* logger;

        SemanticContext* get_context() const;
        void set_context(SemanticContext* newContext);

    private:
        SemanticContext* context;
    };
}

#endif
