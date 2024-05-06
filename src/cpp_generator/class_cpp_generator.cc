#include "cpp_generator/class_cpp_generator.h"
#include "cpp_generator/function_cpp_generator.h"

using namespace haard;

void ClassCppGenerator::build(Class* klass) {
    build_header(klass);
}

void ClassCppGenerator::build_header(Class* klass) {
    header << "class " << klass->get_name().get_value();

    if (klass->get_super_type()) {

    }

    header << " {\npublic:\n";

    for (int i = 0; i < klass->variables_count(); ++i) {
        Variable* var = klass->get_variable(i);
        header << "    ";
        header << var->get_type()->to_cpp() << ' ';
        header << var->get_name().get_value() << ";\n";
    }

    if (klass->functions_count() > 0) {
        if (klass->variables_count() > 0) {
            header << "\n";
        }

        for (int i = 0; i< klass->functions_count(); ++i) {
            FunctionCppGenerator fgen;

            fgen.build(klass->get_function(i));
            header << "    " << fgen.get_header();
        }
    }

    header << "};\n";
}

std::string ClassCppGenerator::get_header() {
    return header.str();
}

std::string ClassCppGenerator::get_cpp() {
    return cpp.str();
}
