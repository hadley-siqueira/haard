#include <iostream>
#include "token/token.h"
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "string_pool/string_pool.h"
#include "pretty_printer/pretty_printer.h"
#include "driver/driver.h"
#include "utils/utils.h"

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
    std::cout << colorify("<red>error<normal>: can't define Foo because it is alredy defined\n");
    std::cout << build_message(argv[1], 3, 3, "teste") << '\n';
    std::stringstream tst;
    tst << "<red>error<normal>: can't define class <white>'Foo<T, K>'<normal> because it is already defined on line 70\n"
           "   --> ./file.hd\n"
           "120 | class Foo<T, K>:\n"
           "            ^ tried to define here\n\n"
           "   --> ./file.hd\n"
           " 70 | struct Foo<T, K>:\n"
           "             ^ but conflicts here\n";

    std::cout << colorify(tst.str()) << '\n';
    test_driver(argc, argv);

    return 0;
}
