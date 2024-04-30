#include "cpp_generator/modules_cpp_generator.h"
#include "cpp_generator/module_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void ModulesCppGenerator::build(Modules* modules) {
    rmdir("cpp");
    mkdir("cpp");

    for (int i = 0; i < modules->modules_count(); ++i) {
        build_module(modules->get_module(i));
    }
}

void ModulesCppGenerator::build_module(Module* module) {
    ModuleCppGenerator gen;

    gen.build(module);
}
