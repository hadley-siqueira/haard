#include <iostream>
#include "token/token.h"
#include "scanner/scanner.h"
#include "scanner/scanner2.h"
#include "parser/parser.h"
#include "string_pool/string_pool.h"
#include "pretty_printer/pretty_printer.h"
#include "driver/driver.h"
#include "utils/utils.h"
#include "json/json.h"

#include <sys/types.h>
#include <unistd.h>

using namespace haard;

void test_scanner(int argc, char** argv) {
    Scanner sc;

    auto tks = sc.read(argv[1]);

    for (auto tk : tks) {
        std::cout << tk.to_str() << '\n';
    }
}

void test_parser(int argc, char** argv) {
    Parser p;

    auto m = p.read(argv[1]);
    delete m;
}

void test_pretty_printer(int argc, char** argv) {
    Parser parser;
    PrettyPrinter printer;

    std::cout << "parsing...\n";
    Ast* module = parser.read(argv[1]);

    std::cout << "printing...\n";
    printer.print(module);
    std::cout << printer.get_output() << '\n';

    delete module;
}

void test_driver(int argc, char** argv) {
    Driver driver;

    driver.run(argc, argv);
    driver.exit();
}

int main(int argc, char** argv) {
    if (argc > 2)
    test_pretty_printer(argc, argv);

    //test_driver(argc, argv);

    return 0;
}
