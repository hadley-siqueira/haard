// Dumps the token stream of a .hd file in a stable text format, one line per
// token: kind, offset, length, the line:column derived from that offset, the
// newline flag, the indentation of the token's line and the lexeme. This is the
// basis of the golden tests: the output is compared against the matching file
// in expected/.
//
// The scanner's diagnostics are printed before the tokens and are part of the
// comparison too, so their rendering is covered by the tests as well.
#include <haard/module/module.h>
#include <haard/scanner/scanner.h>
#include <iomanip>
#include <string>
#include <iostream>

using namespace haard;

std::string escape(const std::string_view& s) {
    std::string r;

    for (char c : s) {
        switch (c) {
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\t': r += "\\t"; break;
            case '\r': r += "\\r"; break;
            default: r += c;
        }
    }

    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: dump_tokens <file.hd>\n";
        return 2;
    }

    Module module;
    Scanner sc;

    sc.set_module(&module);
    sc.get_tokens(argv[1]);

    // the scanner collects its diagnostics instead of printing them, so the
    // driver is what puts them in front of the stream
    module.get_logger()->print(std::cout);

    auto& list = module.get_tokens()->get_tokens();

    for (u32 i = 0; i < list.size(); ++i) {
        auto& tk = list[i];
        auto pos = module.get_source_file()->position_of(tk.get_offset());

        // printing the position derived from the offset puts SourceFile::
        // position_of under the golden tests as well, on every case
        std::cout << std::left << std::setw(26) << tk.get_kind_as_string()
            << " off=" << std::setw(4) << tk.get_offset()
            << " len=" << std::setw(3) << tk.get_length()
            << " at=" << std::setw(7)
            << (std::to_string(pos.line) + ":" + std::to_string(pos.column))
            << " nl=" << tk.get_newline_before()
            << " ws=" << std::setw(3) << (int) tk.get_whitespace()
            << " '" << escape(module.get_token_value(i)) << "'\n";
    }

    return 0;
}
