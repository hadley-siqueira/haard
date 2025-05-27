#include <iostream>

#include <haard/string_pool/string_pool.h>

int main(int argc, char* argv[]) {
    std::cout << haard::StringPool::get("Hello, world!") << std::endl;
    return 0;
}
