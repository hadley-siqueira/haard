// Checks properties that must hold for ANY input, without depending on an
// expected output. This complements the golden tests: it catches regressions in
// new files, for which nobody has written an expected/ yet.
//
//  round-trip  each token's lexeme is exactly source[offset, length), the
//              tokens are in order, do not overlap, and whatever sits between
//              two tokens is only whitespace, a comment, or a byte a
//              diagnostic points at. That is, no code byte is lost or
//              duplicated by the scanner without a word about it.
//
//  whitespace  the indentation counter matches the spaces at the start of the
//              line where the token begins.
//
//  newline     a token carries the newline flag exactly when a '\n' sits
//              between the end of the previous token and its own start. The
//              first token of the file always carries it: nothing precedes it.
//
//  position    the line and column SourceFile::position_of derives from an
//              offset match the ones counted here from the raw text, columns
//              in characters rather than bytes.
//
//  eof         the stream ends with a TK_EOF that sits at the end of the file,
//              is empty, has zero indentation so the parser's blocks all close,
//              and appears nowhere else. Its newline flag is not special: it
//              follows the rule above like any other token.
#include <haard/module/module.h>
#include <haard/scanner/scanner.h>
#include <iostream>
#include <vector>

using namespace haard;

int fails = 0;

void fail(const std::string& what) {
    ++fails;
    std::cout << "    " << what << "\n";
}

// line (1-based), indentation and column (1-based, in characters) of the offset,
// counted from the raw text and independently of SourceFile
void line_indent_and_column(const std::string& src, u32 offset, int& line,
                            int& indent, int& column) {
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

    column = 1;

    for (size_t i = bol; i < offset && i < src.size(); ++i) {
        if ((((unsigned char) src[i]) & 0xC0) != 0x80) {
            ++column;
        }
    }
}

bool holds_newline(const std::string& src, size_t from, size_t to) {
    for (size_t i = from; i < to && i < src.size(); ++i) {
        if (src[i] == '\n') {
            return true;
        }
    }

    return false;
}

// whatever separates two tokens can only be whitespace, a comment, or a byte
// some diagnostic points at — an invalid character is dropped from the stream,
// but never in silence
void check_gap(const std::string& src, const std::vector<bool>& reported,
               size_t from, size_t to) {
    size_t i = from;

    while (i < to) {
        if (src[i] == '#') {
            while (i < to && src[i] != '\n') {
                ++i;
            }
        } else if (src[i] == ' ' || src[i] == '\t' || src[i] == '\n' || src[i] == '\r') {
            ++i;
        } else if (reported[i]) {
            ++i;
        } else {
            fail("code byte dropped between tokens with no diagnostic, at offset "
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

    Module module;
    Scanner sc;

    sc.set_module(&module);
    sc.get_tokens(argv[1]);

    auto& src = module.get_source_file()->get_content();
    auto& list = module.get_tokens()->get_tokens();

    // every byte a diagnostic points at, so a gap can be told from a hole
    std::vector<bool> reported(src.size(), false);

    for (auto& log : module.get_logger()->get_logs()) {
        for (u32 i = log.get_offset();
             i < log.get_offset() + log.get_length() && i < src.size(); ++i) {
            reported[i] = true;
        }
    }

    size_t end_of_previous = 0;

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
            check_gap(src, reported, end_of_previous, offset);
        }

        if (std::string(module.get_token_value(i)) != src.substr(offset, length)) {
            fail(at + ": lexeme does not match the original text");
        }

        // the first token has nothing before it, so it always carries the flag
        bool expected_newline = i == 0
            || holds_newline(src, end_of_previous, offset);

        if (tk.get_newline_before() != expected_newline) {
            fail(at + std::string(": newline_before is ")
                    + (tk.get_newline_before() ? "set" : "clear")
                    + " but the text before it "
                    + (expected_newline ? "holds" : "holds no") + " line break");
        }

        end_of_previous = offset + length;

        int line = 0;
        int indent = 0;
        int column = 0;
        line_indent_and_column(src, offset, line, indent, column);

        // 'ws' holds the indentation of the line, saturated at 127 because the
        // field is 7 bits wide — the scanner reports the overflow rather than
        // truncating in silence. TK_EOF carries a forced zero, checked below
        int expected_ws = indent > 127 ? 127 : indent;

        if (tk.get_kind() != TK_EOF && tk.get_whitespace() != expected_ws) {
            fail(at + ": ws=" + std::to_string(tk.get_whitespace())
                    + " but line " + std::to_string(line) + " has "
                    + std::to_string(indent) + " spaces");
        }

        auto position = module.get_source_file()->position_of(offset);

        if (position.line != (u32) line || position.column != (u32) column) {
            fail(at + ": position_of says "
                    + std::to_string(position.line) + ":"
                    + std::to_string(position.column) + " but the text says "
                    + std::to_string(line) + ":" + std::to_string(column));
        }

        if (tk.get_kind() == TK_EOF && i + 1 != list.size()) {
            fail(at + ": TK_EOF before the end of the stream");
        }
    }

    check_gap(src, reported, end_of_previous, src.size());

    if (list.size() == 0) {
        fail("the stream is empty: it must always end with a TK_EOF");
    } else {
        auto& last = list[list.size() - 1];

        if (last.get_kind() != TK_EOF) {
            fail("the stream does not end with TK_EOF");
        }

        if (last.get_offset() != src.size() || last.get_length() != 0) {
            fail("TK_EOF is not the empty token at the end of the file");
        }

        // a nonzero indentation here would keep the parser's blocks open
        if (last.get_whitespace() != 0) {
            fail("TK_EOF has a nonzero indentation");
        }
    }

    return fails;
}
