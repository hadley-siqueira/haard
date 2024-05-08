#include "cpp_generator/function_cpp_generator.h"

using namespace haard;

void FunctionCppGenerator::build(Function* function) {
    build_header(function);
    build_cpp(function);
}

void FunctionCppGenerator::build_header(Function* function) {
    std::string signature = get_signature(function);

    header << signature << ";\n";
}

void FunctionCppGenerator::build_cpp(Function* function) {
    std::string signature = get_signature(function);

    cpp << signature << " {\n\n}\n\n";
}

std::string FunctionCppGenerator::get_signature(Function* function) {
    std::stringstream ss;

    ss << function->get_return_type()->to_cpp() << ' ';
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
