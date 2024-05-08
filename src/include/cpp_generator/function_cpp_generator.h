#ifndef HAARD_FUNCTION_CPP_GENERATOR_H
#define HAARD_FUNCTION_CPP_GENERATOR_H

#include <sstream>

#include "ast/function.h"

namespace haard {
    class FunctionCppGenerator {
    public:
        void build(Function* function);
        void build_header(Function* function);
        void build_cpp(Function* function);

        std::string get_signature(Function* function);
        std::string get_header();
        std::string get_cpp();

    private:
        std::stringstream header;
        std::stringstream cpp;
    };
}

#endif
