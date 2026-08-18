// Checks properties that must hold for ANY input, without depending on an
// expected output. This complements the golden tests: it catches regressions in
// new files, for which nobody has written an expected/ yet.
//
//  round-trip  each token's lexeme is exactly source[offset, length), the
//              tokens are in order, do not overlap, and whatever sits between
//              two tokens is only whitespace or a comment. That is, no code
//              byte is lost or duplicated by the scanner.
//
//  whitespace  the indentation counter matches the spaces at the start of the
//              line where the token begins; tokens on the same line share the
//              flag bit; tokens on different lines have it flipped.
#include <haard/context/context.h>
#include <haard/scanner/scanner.h>
#include <iostream>

using namespace haard;

int fails = 0;

void fail(const std::string& what) {
    ++fails;
    std::cout << "    " << what << "\n";
}

// line (1-based) and indentation of the line the offset falls in, read from
// the raw text
void line_and_indent(const std::string& src, u32 offset, int& line, int& indent) {
    size_t bol = 0;

    line = 1;

    for (size_t i = 0; i < offset && i < src.size(); ++i) {
        if (src[i] == '\n') {
            ++line;
            bol = i + 1;
        }
    }

    indent = 0;

    while (bol + indent < src.size() && src[bol + indent] == ' ') {
        ++indent;
    }
}

// whatever separates two tokens can only be whitespace or a comment
void check_gap(const std::string& src, size_t from, size_t to) {
    size_t i = from;

    while (i < to) {
        if (src[i] == '#') {
            while (i < to && src[i] != '\n') {
                ++i;
            }
        } else if (src[i] == ' ' || src[i] == '\t' || src[i] == '\n' || src[i] == '\r') {
            ++i;
        } else {
            fail("code byte dropped between tokens at offset "
                 + std::to_string(i) + ": '" + src[i] + "'");
            return;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "usage: check_invariants <file.hd>\n";
        return 2;
    }

    Context ctx;
    Scanner sc;

    sc.set_context(&ctx);
    sc.get_tokens(argv[1]);

    auto& src = ctx.get_source_file()->get_content();
    auto& list = ctx.get_tokens()->get_tokens();

    size_t end_of_previous = 0;
    int previous_line = -1;
    bool previous_flag = false;

    for (u32 i = 0; i < list.size(); ++i) {
        auto& tk = list[i];
        u32 offset = tk.get_offset();
        u32 length = tk.get_length();
        std::string at = "token " + std::to_string(i) + " (offset "
                       + std::to_string(offset) + ")";

        if (offset + length > src.size()) {
            fail(at + ": runs past the end of the file");
            continue;
        }

        if (offset < end_of_previous) {
            fail(at + ": overlaps the previous token");
        } else {
            check_gap(src, end_of_previous, offset);
        }

        if (std::string(ctx.get_token_value(i)) != src.substr(offset, length)) {
            fail(at + ": lexeme does not match the original text");
        }

        end_of_previous = offset + length;

        int line = 0;
        int indent = 0;
        line_and_indent(src, offset, line, indent);

        if (tk.get_whitespace() != indent) {
            fail(at + ": ws=" + std::to_string(tk.get_whitespace())
                    + " but line " + std::to_string(line) + " has "
                    + std::to_string(indent) + " spaces");
        }

        if (previous_line != -1) {
            if (line == previous_line && tk.get_whitespace_flag() != previous_flag) {
                fail(at + ": same line as the previous token but the flag changed");
            } else if (line != previous_line && tk.get_whitespace_flag() == previous_flag) {
                fail(at + ": different line from the previous token but the flag did not flip");
            }
        }

        previous_line = line;
        previous_flag = tk.get_whitespace_flag();
    }

    check_gap(src, end_of_previous, src.size());
    return fails;
}
