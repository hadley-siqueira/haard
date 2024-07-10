#ifndef HAARD_AST_PARAMETER_H
#define HAARD_AST_PARAMETER_H

#include <string>

namespace haard::ast {
    class Parameter {
    public:
        const std::string &get_name() const;
        void set_name(const std::string& name);

    private:
        std::string name;
    };
}

#endif
