#include "driver/driver.h"
#include "parser/parser.h"

using namespace haard;

Module* Driver::parse_file(std::string path) {
    if (!modules.has_module(path)) {
        Parser parser;
        modules.add_module(path, parser.read(path));
    }

    return modules.get_module(path);
}
