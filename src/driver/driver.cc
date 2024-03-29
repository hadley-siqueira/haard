#include <iostream>
#include <fstream>

#include "driver/driver.h"
#include "parser/parser.h"

using namespace haard;

Driver::Driver() {
    logger = new Logger();
}

Driver::~Driver() {
    delete logger;
}

void Driver::run(int argc, char** argv) {
    parse_file(argv[1]);
}

void Driver::exit() {
    std::cout << logger->to_str() << std::endl;
    ::exit(0);
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

bool Driver::file_exists(std::string path) {
    std::ifstream f(path.c_str());
    return f.good();
}
