#include <haard/context/context.h>
#include <iostream>

using namespace haard;

Tokens* Context::get_tokens() {
    return &tokens;
}

SourceFile* Context::get_source_file() {
    return &source_file;
}

Ast* Context::get_ast() {
    return &ast;
}

std::string_view Context::get_token_value(u32 token) {
    auto t = tokens.get_token(token);
    std::string_view view(source_file.get_content());

    return view.substr(t.get_offset(), t.get_length());
}

void Context::inspect_tokens() {
    for (auto tk : tokens.get_tokens()) {
        auto offset = tk.get_offset();
        auto length = tk.get_length();

        std::cout << offset << ":" << length << " -> '";

        for (auto i = offset; i < offset + length; ++i) {
            std::cout << source_file.char_at(i);
        }

        std::cout << "' (" << tk.get_kind_as_string() << ")\n";
    }
}

void Context::inspect_ast() {
    u32 index = 0;

    for (auto node : ast.get_nodes()) {
        std::cout << "{\n" 
            << "    index: " << index << ",\n"
            << "    kind: " << node.get_kind() << ",\n"
            << "    token: " << node.get_token() << ",\n"
            << "    sibling: " << node.get_sibling() << ",\n"
            << "    children: " << node.get_children() << "\n},\n";

        ++index;
    }
}
