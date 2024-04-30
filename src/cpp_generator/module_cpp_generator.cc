#include <fstream>
#include <iostream>

#include "cpp_generator/module_cpp_generator.h"
#include "cpp_generator/class_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void ModuleCppGenerator::build(Module* module) {
    create_module_directory(module);
    build_module_header(module);
    build_module_cpp(module);
}

void ModuleCppGenerator::create_module_directory(Module* module) {
    int i;
    std::string path = module->get_relative_path();
    auto p = split(path, '.');

    for (i = 0; i < p.size() - 1; ++i) {
        filepath += p[i] + "/";
        std::string pp = "cpp/" + filepath;
        std::cout << "PATH = " << pp << '\n';
        mkdir(pp);
    }

    filepath += p[i];
}

void ModuleCppGenerator::build_module_header(Module* module) {
    std::ofstream f("cpp/" + filepath + ".h");

    std::string header = uppercase(join(split(filepath, '/'), "_")) + "_H";
    std::string ns = join(split(filepath, '/'), "::");

    output << "#ifndef " << header << "\n";
    output << "#define " << header << "\n\n";

    output << "namespace " << ns << " {\n";

    build_module_classes(module);

    output << "}\n\n";
    output << "#endif";

    f << output.str();
}

void ModuleCppGenerator::build_module_cpp(Module* module) {
    std::ofstream f("cpp/" + filepath + ".cpp");

    f << "the .cpp";
}

void ModuleCppGenerator::build_module_classes(Module* module) {
    for (int i = 0; i < module->classes_count(); ++i) {
        ClassCppGenerator gen;

        gen.build(module->get_class(i));
        output << indent(gen.get_header(), 4) << '\n';
    }
}
