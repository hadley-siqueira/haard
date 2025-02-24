#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <string>
#include <map>

#include "haard/log/logger.h"

#include "haard/ast/ast.h"
#include "haard/ast/modules.h"
#include "haard/ast/module.h"

namespace haard {
    enum DriverCommands {
        DRIVER_CMD_PRETTY_PRINT,
        DRIVER_CMD_JSON,
    };

    class Driver {
    public:
        Driver();
        ~Driver();

        void parse_args(int argc, char** argv);
        void run(int argc, char** argv);
        void exec_commands();
        void configure();
        void exit();
        void read_configuration(std::string path);

    public:
        void semantic_analysis();

    public:
        AstNode* parse_file(std::string path);
        void parse_module_imports(AstNode* module);
        void parse_import(AstNode* import);
        void parse_simple_import(AstNode* import);
        std::string build_import_path(AstNode* import);
        std::string build_relative_path(std::string path);
        void configure_search_path();
        void set_root_path_from_main_file();

    public:
        void pretty_print();
        void print_json();
        void print_logs();

    private:
        bool file_exists(std::string path);

    private:
        Modules modules;
        Module* module;

    // system related stuff
    private:
        std::vector<DriverCommands> commands;
        std::vector<std::string> search_path;
        std::string configuration_file_path;
        std::string main_path;
        std::string root_path;
        char path_delimiter;

    // flags
    private:
        bool show_logs_flag;
    };
}

#endif
