#ifndef HAARD_FUNCTION_TYPE_H
#define HAARD_FUNCTION_TYPE_H

#include "ast/types/type.h"
#include "ast/type_list.h"

namespace haard {
    class FunctionType : public Type {
    public:
        FunctionType(TypeList* params_type, Type* return_type);

    public:
        Type* get_return_type() const;
        void set_return_type(Type* newReturn_type);

        TypeList* get_param_types() const;
        void set_param_types(TypeList* newParam_types);

    private:
        TypeList* param_types;
        Type* return_type;
    };
}

#endif
