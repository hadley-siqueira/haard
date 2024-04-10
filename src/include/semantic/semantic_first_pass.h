#ifndef HAARD_SEMANTIC_FIRST_PASS_H
#define HAARD_SEMANTIC_FIRST_PASS_H

#include "semantic/semantic_pass.h"
#include "ast/ast.h"

namespace haard {
    class SemanticFirstPass : public SemanticPass {
    public:
        void build_modules(Modules* modules);
        void build_modules_function(Modules* modules);

        void build_module_functions(Module* module);
        void build_function(Function* function);

    private:
        void define_function(Function* function);
        Function* check_for_overloaded(Symbol* sym, Function* function);
    };
}

#endif
