#include <iostream>
#include <haard/source_file/source_file.h>
#include <haard/scanner/scanner.h>
#include <haard/context/context.h>

int main(int argc, char* argv[]) {
    haard::Context ctx;
    haard::Scanner sc;
    sc.set_context(&ctx);

    sc.get_tokens(argv[1]);
    ctx.inspect_tokens();

    return 0;
}
