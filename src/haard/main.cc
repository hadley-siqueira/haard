#include <iostream>
#include <haard/driver/driver.h>

int main(int argc, char* argv[]) {
    haard::Driver driver;

    driver.run(argc, argv);
    return 0;
}
