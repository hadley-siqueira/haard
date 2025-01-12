#ifndef HAARD_SEMANTIC_ANALYZER_H
#define HAARD_SEMANTIC_ANALYZER_H

#include "haard/ast/ast.h"

namespace haard {
    class SemanticAnalyzer {
    public:
        SemanticAnalyzer();
        ~SemanticAnalyzer();

    public:
        void analyze_module(Module* module);

        void define_module_classes(Module* module);

        std::string get_name(AstNode* node);
    private:
    };
}

#endif
