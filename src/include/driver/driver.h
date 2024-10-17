#ifndef HAARD_DRIVER_H
#define HAARD_DRIVER_H

#include <string>
#include <map>

#include "log/logger.h"

#include "haard/ast/ast.h"
#include "haard/modules_manager/modules_manager.h"

namespace haard {
    enum DriverCommands {
        DRIVER_CMD_PRETTY_PRINT,
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
        Ast* parse_file(std::string path);
        void parse_module_imports(Ast* module);
        void parse_import(Ast* import);
        void parse_simple_import(Ast* import);
        std::string build_import_path(Ast* import);
        std::string build_relative_path(std::string path);
        void configure_search_path();
        void set_root_path_from_main_file();

    public:
        void pretty_print();

    private:
        bool file_exists(std::string path);

    private:
        ModulesManager modules;
        Ast* module;

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
