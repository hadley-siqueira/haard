#include <iostream>
#include "token/token.h"
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "string_pool/string_pool.h"
#include "pretty_printer/pretty_printer.h"
#include "driver/driver.h"
#include "utils/utils.h"
#include "json/json.h"

using namespace haard;

void test_scanner(int argc, char** argv) {
    Scanner sc;

    auto tks = sc.read(argv[1]);

    for (auto tk : tks) {
        std::cout << tk.to_str() << '\n';
    }
}

void test_json() {
    Json* function = new Json();
    Json* module = new Json();
    Json* cloned;

    function->set("line", 10);
    function->set("column", 20);
    function->set("name", "my_function");
    function->add("parameters", 2);
    function->add("parameters", 3);
    function->add("parameters", "hello");

    cloned = function->clone();

    module->set("path", "/home/foo.hd");
    module->add("functions", function);

    std::cout << function->to_str() << "\n";
    std::cout << cloned->to_str() << "\n";
    std::cout << module->to_str() << "\n";

    delete module;
    delete cloned;
}

void test_parser(int argc, char** argv) {
    Parser p;

    auto m = p.read(argv[1]);
    delete m;
}

void test_pretty_printer(int argc, char** argv) {
    Parser parser;
    PrettyPrinter printer;

    Module* module = parser.read(argv[1]);
    printer.print_module(module);
    std::cout << printer.get_output() << '\n';

    delete module;
}

void test_driver(int argc, char** argv) {
    Driver driver;

    driver.run(argc, argv);
    driver.exit();
}

int main(int argc, char** argv) {
    //test_scanner(argc, argv);
    //test_pretty_printer(argc, argv);
    //test_driver(argc, argv);
    test_json();

    return 0;
}
