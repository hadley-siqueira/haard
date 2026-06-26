#include <iostream>
#include <haard/source_file/source_file.h>
#include <haard/scanner/scanner.h>
#include <haard/parser/parser.h>
#include <haard/context/context.h>
#include <haard/pretty_printer/pretty_printer.h>

void test_scanner(int argc, char* argv[]) {
    haard::Context ctx;
    haard::Scanner sc;
    sc.set_context(&ctx);

    sc.get_tokens(argv[1]);
    ctx.inspect_tokens();
}

void test_parser(int argc, char* argv[]) {
    std::cout << "Testing parser...\n\n";
    haard::Context ctx;
    haard::Parser parser;
    parser.set_context(&ctx);

    parser.parse_file(argv[1]);
    ctx.inspect_tokens();
    ctx.inspect_ast();
}

void test_pretty_printer(int argc, char* argv[]) {
    std::cout << "Testing pretty printer...\n\n";

    haard::Context ctx;
    haard::Parser parser;
    haard::PrettyPrinter printer;

    parser.set_context(&ctx);
    printer.set_context(&ctx);

    parser.parse_file(argv[1]);
    printer.print();
}

int main(int argc, char* argv[]) {
    // test_scanner(argc, argv);
    // test_parser(argc, argv);
    test_pretty_printer(argc, argv);

    return 0;
}
