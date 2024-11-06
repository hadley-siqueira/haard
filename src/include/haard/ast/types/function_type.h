#ifndef HAARD_AST_FUNCTION_TYPE_H
#define HAARD_AST_FUNCTION_TYPE_H

#include <vector>

#include "haard/ast/types/type.h"

namespace haard {
    class FunctionType : public Type {
    public:
        FunctionType();
        FunctionType(std::vector<Type*>& parameters_type, Type* return_type);
        ~FunctionType();

    public:

        const std::vector<Type*>& get_parameters_type() const;
        void set_parameters_type(const std::vector<Type*>& parameters_type);

        Type* get_return_type() const;
        void set_return_type(Type* return_type);

        virtual std::string to_json();
        virtual std::string to_str();

    private:
        std::vector<Type*> parameters_type;
        Type* return_type;
    };
}

#endif
