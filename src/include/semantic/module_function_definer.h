#ifndef HAARD_MODULE_FUNCTION_DEFINER_H
#define HAARD_MODULE_FUNCTION_DEFINER_H

#include "log/logger.h"
#include "semantic/semantic_pass.h"
#include "ast/module.h"

namespace haard {
    class ModuleFunctionDefiner : public SemanticPass {
    public:
        void build(Module* module);
        void define_function(Function* function);
        void link_parameters_type(Function* function);

        void define_function_in_scope(Function* function);
        bool check_for_function_redefinition(Function* f1, Function* f2);

    private:
        Module* module;
    };
}

#endif
