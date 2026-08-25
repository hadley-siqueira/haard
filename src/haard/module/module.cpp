#include <haard/module/module.h>
#include <iostream>

using namespace haard;

TokenStream* Module::get_tokens() {
    return &tokens;
}

SourceFile* Module::get_source_file() {
    return &source_file;
}

Logger* Module::get_logger() {
    // the logger renders a diagnostic from an offset, so it needs the file
    // those offsets belong to
    logger.set_source_file(&source_file);

    return &logger;
}

Ast* Module::get_ast() {
    return &ast;
}

std::string_view Module::get_token_value(u32 token) {
    auto t = tokens.get_token(token);
    std::string_view view(source_file.get_content());

    return view.substr(t.get_offset(), t.get_length());
}

void Module::inspect_tokens() {
    for (auto tk : tokens.get_tokens()) {
        auto offset = tk.get_offset();
        auto length = tk.get_length();

        std::cout << offset << ":" << length << " -> '";

        for (auto i = offset; i < offset + length; ++i) {
            std::cout << source_file.char_at(i);
        }

        std::cout << "' (" << tk.get_kind_as_string()
            << ", newline_before=" << tk.get_newline_before()
            << ", ws=" << (int) tk.get_whitespace() << ")\n";
    }
}

void Module::inspect_ast() {
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
