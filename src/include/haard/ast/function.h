#ifndef HAARD_AST_FUNCTION_H
#define HAARD_AST_FUNCTION_H

#include <vector>
#include <string>

#include "haard/ast/parameter.h"

namespace haard::ast {
    class Function {
    public:
        void add_parameter(Parameter* parameter);
        Parameter* get_parameter(size_t idx);
        size_t parameters_count();

        const std::string& get_name() const;
        void set_name(const std::string& name);

    private:
        int line;
        int column;
        std::string name;
        std::vector<Parameter*> parameters;
    };
}

#endif
