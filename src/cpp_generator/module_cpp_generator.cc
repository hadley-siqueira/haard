#include <fstream>
#include <iostream>

#include "cpp_generator/module_cpp_generator.h"
#include "cpp_generator/class_cpp_generator.h"
#include "cpp_generator/function_cpp_generator.h"
#include "utils/utils.h"

using namespace haard;

void ModuleCppGenerator::build(Module* module) {
    create_module_directory(module);

    build_module_classes(module);
    build_module_functions(module);

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
        mkdir(pp);
    }

    filepath += p[i];
}

void ModuleCppGenerator::build_module_header(Module* module) {
    std::ofstream f("cpp/" + filepath + ".h");

    std::string header_h = uppercase(join(split(filepath, '/'), "_")) + "_H";
    std::string ns = join(split(filepath, '/'), "::");

    f << "#ifndef " << header_h << "\n";
    f << "#define " << header_h << "\n\n";

    f << "#include \"haard.h\"\n\n";

    f << build_module_imports(module);

    f << "namespace " << module->get_cpp_namespace() << " {\n";

    f << indent(header.str(), 4);

    f << "}\n\n";
    f << "#endif";
}

void ModuleCppGenerator::build_module_cpp(Module* module) {
    std::ofstream f("cpp/" + filepath + ".cpp");

    f << "#include \"" << filepath << ".h\"\n\n";
    f << imports_cpp.str() << '\n';
    f << cpp.str();

    if (main_function != nullptr) {
        f << "int main(int argc, char** argv) {\n";
        f << "    " << main_function->get_cpp_namespace() << "(argc, argv);\n" << "}\n";
    }
}

std::string ModuleCppGenerator::build_module_imports(Module* module) {
    std::stringstream imports;

    if (module->imports_count() > 0) {
        for (int i = 0; i < module->imports_count(); ++i) {
            Import* imp = module->get_import(i);
            std::string path = imp->get_module()->get_cpp_path();
            path.pop_back(); // remove cpp
            path.pop_back();
            path.pop_back();
            path += 'h';
            imports << "#include \"" << path << "\"\n";
        }

        imports << "\n\n";

        for (int i = 0; i < module->imports_count(); ++i) {
            Import* imp = module->get_import(i);
            imports_cpp << "using namespace " << imp->get_module()->get_cpp_namespace() << ";\n";
        }

        imports << '\n';
    }


    return imports.str();
}

void ModuleCppGenerator::build_module_classes(Module* module) {
    for (int i = 0; i < module->classes_count(); ++i) {
        ClassCppGenerator gen;

        gen.build(module->get_class(i));
        header << gen.get_header() << '\n';
        cpp << gen.get_cpp();
    }
}

void ModuleCppGenerator::build_module_functions(Module* module) {
    main_function = nullptr;

    for (int i = 0; i < module->functions_count(); ++i) {
        FunctionCppGenerator gen;
        Function* function = module->get_function(i);

        gen.build(function);
        header << gen.get_header() << '\n';
        cpp << gen.get_cpp();

        std::string name = function->get_name().get_value();

        if (name == "main") {
            main_function = function;
        }
    }
}

