#include <iostream>
#include <vector>
#include "token/token.h"
#include "scanner/scanner.h"
#include "parser/parser.h"
#include "defs.h"
#include "ast/ast.h"
#include "printer/printer.h"

using namespace haard;

void test_scanner(std::string path) {
    Scanner s;

    std::vector<Token> tokens = s.read(path);

    for (int i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i].to_str() << std::endl;
    }
}

void test_parser(std::string path) {
    Parser parser;
    Printer printer;
    Source* src = parser.read(path);
    printer.print_source(src);

    std::cout << printer.to_str();

    delete src;
}

int main(int argc, char* argv[]) {
    // test_scanner(argv[1]);
    test_parser(argv[1]);   

    return 0;
}
