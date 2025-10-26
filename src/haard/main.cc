#include <iostream>
#include <haard/scanner/scanner.h>


int main(int argc, char* argv[]) {
    haard::Scanner sc;

    auto tokens = sc.get_tokens(argv[1]);

    for (auto tk : tokens) {
        std::cout << tk.get_value() << std::endl;
    }

    return 0;
}
