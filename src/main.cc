#include <iostream>

#include <haard/string_pool/string_pool.h>
#include <haard/scanner/scanner.h>

void test_scanner(int argc, char* argv[]) {
    haard::Scanner sc;

    auto tokens = sc.read(argv[1]);

    for (auto token : tokens) {
        std::cout << token.to_json() << '\n';
    }
}

int main(int argc, char* argv[]) {
    test_scanner(argc, argv);

    return 0;
}
