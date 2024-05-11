#include "cpp_generator/function_cpp_generator.h"
#include "ast/named_type_descriptor.h"
#include "ast/named_type.h"

using namespace haard;

void FunctionCppGenerator::build(Function* function) {
    build_header(function);
    build_cpp(function);
}

void FunctionCppGenerator::build_header(Function* function) {
    std::string signature = get_signature(function);

    header << function->get_return_type()->to_cpp() << ' ';
    header << signature << ";\n";
}

void FunctionCppGenerator::build_cpp(Function* function) {
    std::string signature = get_signature(function);
    std::string ns;

    if (function->is_method()) {
        int i;
        NamedType* named;
        NamedTypeDescriptor* desc = function->get_named_type_descriptor();

        /*if (desc->get_generics()) {
            cpp << "template <";

            for (i = 0; i < desc->get_generics()->types_count() - 1; ++i) {
                named = (NamedType*) desc->get_generics()->get_type(i);
                cpp << "typename " << named->get_identifier()->get_name().get_value() << ", ";
            }

            named = (NamedType*) desc->get_generics()->get_type(i);
            cpp << "typename " << named->get_identifier()->get_name().get_value() << ">\n";
        }*/

        if (!desc->get_generics()) {
            ns = function->get_named_type_descriptor()->get_name().get_value();
            ns = ns + "::";
        }
    }

    cpp << function->get_return_type()->to_cpp() << ' ';
    cpp << ns << signature << " {\n\n}\n\n";
}

std::string FunctionCppGenerator::get_signature(Function* function) {
    std::stringstream ss;

    ss << function->get_name().get_value() << "(";

    if (function->parameters_count() > 0) {
        int i;
        Variable* param;

        for (i = 0; i < function->parameters_count() - 1; ++i) {
            param = function->get_parameter(i);

            ss << param->get_type()->to_cpp() << ' '
               << param->get_name().get_value() << ", ";
        }

        param = function->get_parameter(i);

        ss << param->get_type()->to_cpp() << ' '
           << param->get_name().get_value();
    }

    ss << ")";
    return ss.str();
}

std::string FunctionCppGenerator::get_header() {
    return header.str();
}

std::string FunctionCppGenerator::get_cpp() {
    return cpp.str();
}
