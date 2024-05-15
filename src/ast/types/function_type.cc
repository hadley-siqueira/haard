#include "ast/types/function_type.h"

using namespace haard;

FunctionType::FunctionType(TypeList* params_type, Type* return_type) {
    set_kind(TYPE_FUNCTION);
    set_param_types(params_type);
    set_return_type(return_type);
}

Type* FunctionType::get_return_type() const {
    return return_type;
}

void FunctionType::set_return_type(Type* newReturn_type) {
    return_type = newReturn_type;
}

TypeList* FunctionType::get_param_types() const {
    return param_types;
}

void FunctionType::set_param_types(TypeList* newParam_types) {
    param_types = newParam_types;
}
