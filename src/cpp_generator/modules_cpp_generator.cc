#include <sstream>
#include <fstream>

#include "cpp_generator/modules_cpp_generator.h"
#include "cpp_generator/module_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void ModulesCppGenerator::build(Modules* modules) {
    //rmdir("cpp");
    mkdir("cpp");

    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module(modules->get_module(i));
    }

    generate_cmake(modules);
}

void ModulesCppGenerator::build_module(Module* module) {
    ModuleCppGenerator gen;

    gen.build(module);
}

void ModulesCppGenerator::generate_cmake(Modules* modules) {
    std::stringstream output;
    std::ofstream f("cpp/CMakeLists.txt");

    output << "cmake_minimum_required(VERSION 3.0)\n";
    output << "project(hdc)\n\n";

    output << "include_directories(.)\n\n";
    output << "set(SOURCES\n";

    for (int i = 0; i < modules->modules_count(); ++i) {
        output << "    " << modules->get_module(i)->get_cpp_path() << '\n';
    }

    output << ")\n\n";
    output << "add_executable(${PROJECT_NAME} ${SOURCES})\n";

    f << output.str();
}
