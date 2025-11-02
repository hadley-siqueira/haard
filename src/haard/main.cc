#include <iostream>
#include <haard/scanner/scanner.h>

int main(int argc, char* argv[]) {
    haard::Scanner sc;

    auto tokens = sc.get_tokens(argv[1]);

    for (auto tk : tokens) {
        std::cout << tk.to_json() << std::endl;
    }

    return 0;
}
