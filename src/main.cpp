#include <iostream>
#include <haard/source_file/source_file.h>
#include <haard/scanner/scanner.h>
#include <haard/parser/parser.h>
#include <haard/context/context.h>

void test_scanner(int argc, char* argv[]) {
    haard::Context ctx;
    haard::Scanner sc;
    sc.set_context(&ctx);

    sc.get_tokens(argv[1]);
    ctx.inspect_tokens();
}

void test_parser(int argc, char* argv[]) {
    std::cout << "Testing parser...\n\n";
    haard::Parser parser;

    parser.parse_file(argv[1]);
}

int main(int argc, char* argv[]) {
    test_scanner(argc, argv);
    test_parser(argc, argv);

    return 0;
}
