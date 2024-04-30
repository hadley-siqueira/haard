#include "cpp_generator/class_cpp_generator.h"

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
        header << "    int " <<
        klass->get_variable(i)->get_name().get_value() << ";\n";
    }

    header << "};\n";
}

std::string ClassCppGenerator::get_header() {
    return header.str();
}

std::string ClassCppGenerator::get_cpp() {
    return cpp.str();
}
