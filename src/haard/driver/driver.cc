#include <iostream>
#include <fstream>
#include <cstring>

#include "haard/driver/driver.h"
#include "haard/parser/parser.h"
#include "haard/pretty_printer/pretty_printer.h"
#include "haard/semantic/semantic_analyzer.h"
#include "haard/log/logs.h"

using namespace haard;

Driver::Driver() {
    path_delimiter = '/';
    configuration_file_path = "...";

    show_logs_flag = true;
}

Driver::~Driver() {

}

void Driver::parse_args(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        if (strstr(argv[i], ".hd") != nullptr) {
            main_path = std::string(argv[i]);
        } else if (strcmp(argv[i], "--help") == 0) {

        } else if (strcmp(argv[i], "--pretty") == 0) {
            commands.push_back(DRIVER_CMD_PRETTY_PRINT);
        } else if (strcmp(argv[i], "--json") == 0) {
            commands.push_back(DRIVER_CMD_JSON);
        } else if (strcmp(argv[i], "--no-logs") == 0) {
            show_logs_flag = false;
        }
    }
}

void Driver::run(int argc, char** argv) {
    if (argc == 1) {
        return;
    }

    parse_args(argc, argv);
    configure();

    if (log_has_error()) {
        exit();
    }

    //parse_module_imports(parse_file(main_path));
    parse_file(main_path);
    exec_commands();
    semantic_analysis();
    exit();
}

void Driver::exec_commands() {
    for (int i = 0; i < commands.size(); ++i) {
        switch (commands[i]) {
        case DRIVER_CMD_PRETTY_PRINT:
            pretty_print();
            break;

        case DRIVER_CMD_JSON:
            print_json();
            break;
        }
    }
}

void Driver::configure() {
    read_configuration("/tmp/foo.txt");
    configure_search_path();
}

void Driver::exit() {
    print_logs();
    ::exit(0);
}

void Driver::read_configuration(std::string path) {
    std::ifstream config_file(path);

    if (config_file.good()) {
        log_info("configuration file opened");
    } else {
        log_info("failed config\n");
    }
}

void Driver::semantic_analysis() {
    SemanticAnalyzer analyzer;

    analyzer.analyze_module(module);
}

AstNode* Driver::parse_file(std::string path) {
    if (!file_exists(path)) {
        log_error("file '" + path + "' couldn't be opened");
        exit();
    }

    module = modules.get_module_by_path(path);

    if (module == nullptr) {
        Parser parser;

        log_info("parsing file " + path);
        module = parser.read(path, build_relative_path(path));
        modules.add_module(path, module);

    }

    return module;
}

void Driver::parse_module_imports(AstNode* module) {
    if (module == nullptr) {
        return;
    }

    std::vector<AstNode*> imports = module->get_children(AST_IMPORT);

    for (int i = 0; i < imports.size(); ++i) {
        parse_import(imports[i]);
    }
}

void Driver::parse_import(AstNode* import) {
    parse_simple_import(import);
}

void Driver::parse_simple_import(AstNode* import) {
    AstNode* module = nullptr;
    std::string path = build_import_path(import);

    module = modules.get_module_by_path(path);

    if (module == nullptr) {
        module = parse_file(path);
        parse_module_imports(module);
    }
}

std::string Driver::build_import_path(AstNode* import) {
    std::string str;
    std::vector<AstNode*> path = import->get_child(AST_IMPORT_PATH)->get_children(AST_IMPORT_PATH_MEMBER);

    for (int i = 0; i < path.size(); ++i) {
        str += path_delimiter;
        str += path[i]->get_value();
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

std::string Driver::build_relative_path(std::string path) {
    int i;
    int j;
    bool found;
    std::string s;

    for (i = 0; i < search_path.size(); ++i) {
        found = true;

        for (j = 0; j < search_path[i].size(); ++j) {
            if (search_path[i][j] != path[j]) {
                found = false;
            }
        }

        if (found) {
            // -3 to remove '.hd'
            for (j = j + 1; j < path.size() - 3; ++j) {
                if (path[j] == path_delimiter) {
                    s += ".";
                } else {
                    s += path[j];
                }
            }

            break;
        }
    }

    return s;
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
        main_path = root_path + path_delimiter + main_path;
    } else {
        for (int i = 0; i < c; ++i) {
            root_path += main_path[i];
        }
    }
}

void Driver::pretty_print() {
    print_logs();

    for (auto it : modules.get_modules()) {
        std::cout << it.second->to_str();
        exit();
    }
}

void Driver::print_json() {
    std::cout << module->to_json() << "\n";
    exit();
}

void Driver::print_logs() {
    if (show_logs_flag) {
        show_logs();
    }
}

bool Driver::file_exists(std::string path) {
    std::ifstream f(path.c_str());
    return f.good();
}
