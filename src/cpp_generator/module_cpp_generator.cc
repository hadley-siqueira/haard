#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "cpp_generator/module_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void ModuleCppGenerator::build(Module* module) {
    create_module_directory(module);
    build_module_header(module);
    build_module_cpp(module);
}

void ModuleCppGenerator::create_module_directory(Module* module) {
    int i;
    std::string path = "cpp." + module->get_relative_path();
    auto p = split(path, '.');

    for (i = 0; i < p.size() - 1; ++i) {
        filepath += p[i] + "/";
        mkdir(filepath.c_str(), 0755);
    }

    filepath += p[i];
    std::cout << filepath << ' ' << path << '\n';
}

void ModuleCppGenerator::build_module_header(Module* module) {
    std::ofstream f(filepath + ".h");

    std::string header = "FOO_BAR_H";
    /**output << "#ifndef " << header << std::endl;
    *output << "#define " << header << std::endl;
    *output << "#endif";*/

    f << header;
}

void ModuleCppGenerator::build_module_cpp(Module* module) {
    std::ofstream f(filepath + ".cpp");

    f << "the .cpp";
}
