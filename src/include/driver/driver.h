#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <string>
#include <map>

#include "log/logger.h"
#include "ast/modules.h"
#include "ast/ast.h"

namespace haard {
    enum DriverCommands {
        DRIVER_CMD_PRETTY_PRINT,
        DRIVER_CMD_CPP
    };

    class Driver {
    public:
        Driver();
        ~Driver();

        void run(int argc, char** argv);
        void exec_commands();
        void configure();
        void exit();
        void read_configuration(std::string path);

    public:
        void semantic_analysis();

    public:
        Module* parse_file(std::string path);
        void parse_module_imports(Module* module);
        void parse_import(Import* import);
        void parse_simple_import(Import* import);
        std::string build_import_path(Import* import);
        std::string build_relative_path(std::string path);
        void configure_search_path();
        void set_root_path_from_main_file();

    public:
        void pretty_print();
        void generate_cpp();

    private:
        bool file_exists(std::string path);

    private:
        Modules modules;
        Ast* ast;

    // system related stuff
    private:
        std::vector<DriverCommands> commands;
        std::vector<std::string> search_path;
        std::string configuration_file_path;
        std::string main_path;
        std::string root_path;
        char path_delimiter;
    };
}

#endif
