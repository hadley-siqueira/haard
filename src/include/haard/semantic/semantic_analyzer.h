#ifndef HAARD_SEMANTIC_ANALYZER_H
#define HAARD_SEMANTIC_ANALYZER_H

#include "haard/ast/ast.h"
#include "haard/semantic/symbol_table.h"

namespace haard {
    class SemanticAnalyzer {
    public:
        SemanticAnalyzer();
        ~SemanticAnalyzer();

    public:
        void analyze_module(Module* module);

        void define_module_classes(Module* module);
        void define_module_functions(Module* module);

        void define_class(Class* klass);
        void define_function(Function* function);

    private:
        Module* current_module;
        SymbolTable* current_symbol_table;
    };
}

#endif
