#include <iostream>

#include <haard/string_pool/string_pool.h>
#include <haard/log/logger.h>
#include <haard/scanner/scanner.h>
#include <haard/parser/parser.h>

void test_scanner(int argc, char* argv[]) {
    haard::Scanner sc;

    auto tokens = sc.read(argv[1]);

    for (auto token : tokens) {
        std::cout << token.to_json() << '\n';
    }
}

void test_parser(int argc, char* argv[]) {
    haard::Parser p;
    haard::Logger logger;

    p.set_logger(&logger);
    auto mod = p.read(argv[1]);
    logger.print_logs();

    delete mod;
}

int main(int argc, char* argv[]) {
    test_parser(argc, argv);

    return 0;
}
