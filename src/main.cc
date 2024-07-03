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

#define N_ITER 200

void test_scanner(int argc, char** argv) {
    Scanner sc;

    auto tks = sc.read(argv[1]);

    for (auto tk : tks) {
        std::cout << tk.to_str() << '\n';
    }
}

void test_scanner3(int argc, char** argv) {
    std::cout << "DEBUG: accessfile() called by process " << ::getpid() << " (parent: " << ::getppid() << ")" << std::endl;
    std::vector<std::vector<Token>> tts;

    for (int i = 0; i < N_ITER; ++i) {
        Scanner sc;

        tts.push_back(sc.read(argv[1]));
    }

    for (int i = 0; i < N_ITER; ++i) {
        if (i == N_ITER - 1) {
            for (auto tk : tts[i]) {
                std::cout << tk.to_str() << '\n';
            }

            std::cout << "i = " << i << '\n';
        }
    }
}


void test_scanner2(int argc, char** argv) {
    std::cout << "DEBUG: accessfile() called by process " << ::getpid() << " (parent: " << ::getppid() << ")" << std::endl;
    std::vector<Json*> tts;
    for (int i = 0; i < N_ITER; ++i) {
        Scanner2 sc;

        auto tks = sc.read(argv[1]);
        tts.push_back(tks);
        //delete tks;
    }

    for (int i = 0; i < N_ITER; ++i) {
        if (i == N_ITER - 1) {
            std::cout << tts[i]->to_str() << std::endl << i << '\n';
        }

        delete tts[i];
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
    if (argc == 3) {
        test_scanner2(argc, argv);
    } else if (argc == 4) {
        test_scanner3(argc, argv);
    }
       // test_pretty_printer(argc, argv);

    //test_driver(argc, argv);
    //test_json();

    return 0;
}
