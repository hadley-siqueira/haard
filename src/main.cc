#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

#include "haard/token/token.h"
#include "haard/scanner/scanner.h"
#include "haard/parser/parser.h"

#include "haard/driver/driver.h"
#include "haard/utils/utils.h"
#include "haard/string_pool/string_pool.h"

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

void test_string_pool(int argc, char** argv) {
    char* v = StringPool::get(argv[1]);

    for (int i = 0; i < 10000000; ++i) {
        std::stringstream ss;
        ss << argv[1];
        ss << i;
        StringPool::get(ss.str().c_str());
    }

    char* k = StringPool::get(argv[1]);

    if (k == v) {
        std::cout << "Ok!\n";
    } else {
        std::cout << "Fail!\n";
    }
}

int main(int argc, char** argv) {
    test_driver(argc, argv);

    return 0;
}
