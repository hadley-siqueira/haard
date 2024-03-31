#include <iostream>
#include <fstream>
#include <cstring>

#include "driver/driver.h"
#include "parser/parser.h"

#include "semantic/semantic_first_pass.h"

#include "cpp_generator/cpp_generator.h"

using namespace haard;

Driver::Driver() {
    path_delimiter = '/';
    configuration_file_path = "...";
    logger = new Logger();
}

Driver::~Driver() {
    delete logger;
}

void Driver::run(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        if (strstr(argv[i], ".hd") != nullptr) {
            main_path = std::string(argv[i]);
        } else if (strcmp(argv[i], "--help") == 0) {

        } else if (strcmp(argv[i], "--cpp") == 0) {
            commands.push_back(DRIVER_CMD_CPP);
        }
    }

    configure();
    parse_module_imports(parse_file(main_path));
    semantic_analysis();

    exec_commands();
}

void Driver::exec_commands() {
    for (int i = 0; i < commands.size(); ++i) {
        switch (commands[i]) {
        case DRIVER_CMD_CPP:
            generate_cpp();
            break;
        }
    }
}

void Driver::configure() {
    read_configuration("/tmp/foo.txt");
    configure_search_path();
}

void Driver::exit() {
    std::cout << logger->to_str() << std::endl;
    ::exit(0);
}

void Driver::read_configuration(std::string path) {
    std::ifstream config_file(path);

    if (config_file.good()) {
        std::cout << "configuration file opened\n";
    } else {
        std::cout << "failed config\n";
    }
}

void Driver::semantic_analysis() {
    SemanticFirstPass pass1;

    pass1.set_logger(logger);
    pass1.build_modules(&modules);
}

Module* Driver::parse_file(std::string path) {
    if (!file_exists(path)) {
        logger->error("file '" + path + "' couldn't be opened");
        exit();
    }

    if (!modules.has_module(path)) {
        Parser parser;

        logger->info("parsing file " + path);
        parser.set_logger(logger);
        modules.add_module(path, parser.read(path));
    }

    return modules.get_module(path);
}

void Driver::parse_module_imports(Module* module) {
    if (module == nullptr) {
        return;
    }

    for (int i = 0; i < module->imports_count(); ++i) {
        parse_import(module->get_import(i));
    }
}

void Driver::parse_import(Import* import) {
    parse_simple_import(import);
}

void Driver::parse_simple_import(Import* import) {
    Module* file = nullptr;
    std::string path = build_import_path(import);

    file = modules.get_module(path);

    if (file != nullptr) {
        import->set_module(file);
    } else {
        file = parse_file(path);
        import->set_module(file);
        parse_module_imports(file);
    }
}

std::string Driver::build_import_path(Import* import) {
    std::string str;

    for (int i = 0; i < import->path_count(); ++i) {
        str += path_delimiter;
        str += import->get_path_token(i).get_value();
    }

    str += ".hd";

    for (int i = 0; i < search_path.size(); ++i) {
        std::string full_path = search_path[i] + str;

        if (file_exists(full_path)) {
            return full_path;
        }
    }

    return str;
}

void Driver::configure_search_path() {
    set_root_path_from_main_file();
    search_path.push_back(root_path);
    search_path.push_back("/home/hadley/.haard/packages/std/0.0.1/src");
}

void Driver::set_root_path_from_main_file() {
    int c;

    for (c = main_path.size() - 1; c >= 0; --c) {
        if (main_path[c] == path_delimiter) break;
    }

    // name.hd
    if (c == -1) {
        root_path = ".";
    } else {
        for (int i = 0; i < c; ++i) {
            root_path += main_path[i];
        }
    }
}

void Driver::generate_cpp() {
    CppGenerator gen;
    std::ofstream file("out.cc");

    gen.build_modules(&modules);
    file << gen.get_output();
}

bool Driver::file_exists(std::string path) {
    std::ifstream f(path.c_str());
    return f.good();
}
