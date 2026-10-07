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

bool Module::is_synthetic(u32 token) {
    return token >= first_synthetic;
}

void Module::bind_by_reference(u32 name_token) {
    by_reference.insert(name_token);
}

bool Module::binds_by_reference(u32 name_token) {
    return by_reference.count(name_token) > 0;
}

void Module::hold_in_temporary(u32 node) {
    temporaries.insert(node);
}

bool Module::held_in_temporary(u32 node) {
    return temporaries.count(node) > 0;
}

void Module::mark_pattern_name(u32 name_token, u32 flag_token) {
    pattern_names[name_token] = flag_token;
}

bool Module::is_pattern_name(u32 name_token) {
    return pattern_names.count(name_token) > 0;
}

u32 Module::flag_of_pattern_name(u32 name_token) {
    auto found = pattern_names.find(name_token);

    return found == pattern_names.end() ? 0 : found->second;
}

void Module::unname(u32 candidate) {
    unnamed.insert(candidate);
}

bool Module::is_unnamed(u32 candidate) {
    return unnamed.count(candidate) > 0;
}

void Module::wait_as_function(u32 node) {
    waiting_functions.insert(node);
}

bool Module::waits_as_function(u32 node) {
    return waiting_functions.count(node) > 0;
}

void Module::wait_as_variant(u32 node, u32 owner, u32 enumeration) {
    waiting_variants[node] = std::make_pair(owner, enumeration);
}

std::pair<u32, u32> Module::waiting_variant(u32 node) {
    auto found = waiting_variants.find(node);

    return found == waiting_variants.end() ? std::make_pair(0u, 0u)
                                           : found->second;
}

void Module::set_pattern_length(u32 for_token, u32 length) {
    pattern_lengths[for_token] = length;
}

u32 Module::get_pattern_length(u32 for_token) {
    auto found = pattern_lengths.find(for_token);

    return found == pattern_lengths.end() ? 0 : found->second;
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
