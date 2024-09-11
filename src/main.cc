#include <iostream>
#include "haard/token/token.h"
#include "haard/scanner/scanner.h"
#include "haard/parser/parser.h"

#include "driver/driver.h"
#include "utils/utils.h"

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

void test_driver(int argc, char** argv) {
    Driver driver;

    driver.run(argc, argv);
    driver.exit();
}

int main(int argc, char** argv) {
    test_driver(argc, argv);

    return 0;
}
