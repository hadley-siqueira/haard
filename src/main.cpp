#include <iostream>
#include <haard/source_file/source_file.h>

int main(int argc, char* argv[]) {
    haard::SourceFile sf;

    sf.open(argv[1]);

    return 0;
}
