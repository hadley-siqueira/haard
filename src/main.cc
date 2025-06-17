#include <iostream>

#include <haard/string_pool/string_pool.h>
#include <haard/log/logger.h>
#include <haard/scanner/scanner.h>
#include <haard/parser/parser.h>
#include <haard/printers/pretty_printer.h>

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

void test_printer(int argc, char* argv[]) {
    haard::Parser p;
    haard::Logger logger;
    haard::PrettyPrinter printer;

    p.set_logger(&logger);
    auto mod = p.read(argv[1]);
    logger.print_logs();

    printer.print(mod);
    std::cout << printer.get_output() << std::endl;

    delete mod;
}

int main(int argc, char* argv[]) {
    test_printer(argc, argv);

    return 0;
}
