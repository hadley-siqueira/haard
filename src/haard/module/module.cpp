#include <haard/module/module.h>
#include <iostream>

using namespace haard;

Module::Module() {
    root = INVALID_ROOT;
    parsed = false;

    // no synthetic token yet, and every real index is below this, so the test
    // in get_token_value is false until one is made
    first_synthetic = 0xffffffff;
}

void Module::set_parsed(bool parsed) {
    this->parsed = parsed;
}

bool Module::is_parsed() {
    return parsed;
}

void Module::set_name(const std::string& name) {
    this->name = name;
}

const std::string& Module::get_name() {
    return name;
}

void Module::set_root(u32 root) {
    this->root = root;
}

u32 Module::get_root() {
    return root;
}

void Module::add_dependency(u32 module, u32 alias) {
    dependencies.push_back(Dependency{module, alias});
}

const std::vector<Dependency>& Module::get_dependencies() {
    return dependencies;
}

TokenStream* Module::get_tokens() {
    return &tokens;
}

SourceFile* Module::get_source_file() {
    return &source_file;
}

StringTable* Module::get_strings() {
    return &strings;
}

SymbolTable* Module::get_symbols() {
    return &symbols;
}

TypeTable* Module::get_types() {
    return &types;
}

ResolutionTable* Module::get_resolutions() {
    return &resolutions;
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
    // a token the lowering pass made: its offset says where to point and not
    // what it says, so the text comes from beside the stream
    if (token >= first_synthetic) {
        return synthetic_text[token - first_synthetic];
    }

    auto t = tokens.get_token(token);
    std::string_view view(source_file.get_content());

    return view.substr(t.get_offset(), t.get_length());
}

u32 Module::add_synthetic_token(TokenKind kind, const std::string& text,
                                u32 like) {
    Token token = tokens.get_token(like);

    token.set_kind(kind);

    if (synthetic_text.size() == 0) {
        first_synthetic = (u32) tokens.size();
    }

    tokens.push(token);
    synthetic_text.push_back(text);

    return first_synthetic + (u32) synthetic_text.size() - 1;
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

u32 Module::find_instantiation(u32 origin,
                               const std::vector<u32>& arguments) {
    for (const Instantiation& made : instantiations) {
        if (made.origin == origin && made.arguments == arguments) {
            return made.made;
        }
    }

    return 0;
}

void Module::add_instantiation(u32 origin, u32 made,
                               const std::vector<u32>& arguments) {
    Instantiation entry;

    entry.origin = origin;
    entry.made = made;
    entry.arguments = arguments;

    instantiations.push_back(entry);
}

const Instantiation* Module::get_instantiation(u32 made) {
    for (const Instantiation& entry : instantiations) {
        if (entry.made == made) {
            return &entry;
        }
    }

    return nullptr;
}
