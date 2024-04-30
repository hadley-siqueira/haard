#include "cpp_generator/class_cpp_generator.h"

using namespace haard;

void ClassCppGenerator::build(Class* klass) {
    build_header(klass);
}

void ClassCppGenerator::build_header(Class* klass) {
    header << "class " << klass->get_name().get_value();

    if (klass->get_super_type()) {

    }

    header << " {\n";

    header << "};\n";
}

std::string ClassCppGenerator::get_header() {
    return header.str();
}
