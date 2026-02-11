#include <haard/scanner/scanner.h>

using namespace haard;

void Scanner::get_tokens(Context& context) {
    tokens = context.get_tokens();
    source_file = context.get_source_file();
}
