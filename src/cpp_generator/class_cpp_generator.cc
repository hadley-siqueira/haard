#include <iostream>

#include "cpp_generator/class_cpp_generator.h"
#include "cpp_generator/function_cpp_generator.h"
#include "ast/named_type.h"

using namespace haard;

void ClassCppGenerator::build(Class* klass) {
    build_class_functions(klass);
    build_header(klass);
    build_cpp(klass);
}

void ClassCppGenerator::build_header(Class* klass) {
    if (klass->get_generics()) {
        int i;
        header << "template <";

        NamedType* named;
        TypeList* types = klass->get_generics();

        for (i = 0; i < types->types_count() - 1; ++i) {
            Type* type = types->get_type(i);

            if (type->get_kind() == TYPE_NAMED) {
                named = (NamedType*) type;
                header << "typename " << named->get_identifier()->get_name().get_value() << ", ";
            }
        }

        Type* type = types->get_type(i);

        if (type->get_kind() == TYPE_NAMED) {
            named = (NamedType*) type;
            header << "typename " << named->get_identifier()->get_name().get_value();
        }

        header << ">\n";
    }

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

        header << functions_header.str();
    }

    header << "};\n";
}

void ClassCppGenerator::build_cpp(Class* klass) {
    cpp << functions_cpp.str();
}

void ClassCppGenerator::build_class_functions(Class* klass) {
    for (int i = 0; i< klass->functions_count(); ++i) {
        FunctionCppGenerator fgen;

        fgen.build(klass->get_function(i));
        functions_header << "    " << fgen.get_header();
        functions_cpp << fgen.get_cpp();
    }
}

std::string ClassCppGenerator::get_header() {
    return header.str();
}

std::string ClassCppGenerator::get_cpp() {
    return cpp.str();
}
