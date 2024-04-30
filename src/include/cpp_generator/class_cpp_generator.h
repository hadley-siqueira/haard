#ifndef HAARD_CLASS_CPP_GENERATOR_H
#define HAARD_CLASS_CPP_GENERATOR_H

#include <sstream>

#include "ast/class.h"

namespace haard {
    class ClassCppGenerator {
    public:
        void build(Class* klass);
        void build_header(Class* klass);

        std::string get_header();
        std::string get_cpp();

    private:
        std::stringstream header;
        std::stringstream cpp;
    };
}

#endif
