#include <haard/ast_query/ast_query.h>

using namespace haard;

AstQuery::AstQuery() {
    module = nullptr;
    ast = nullptr;
}

void AstQuery::set_module(Module* module) {
    this->module = module;
    this->ast = module == nullptr ? nullptr : module->get_ast();
}

std::vector<u32> AstQuery::get_imports() {
    return collect(AST_IMPORT);
}

std::vector<u32> AstQuery::get_functions() {
    return collect(AST_FUNCTION);
}

std::vector<u32> AstQuery::get_classes() {
    return collect(AST_CLASS);
}

std::vector<u32> AstQuery::get_structs() {
    return collect(AST_STRUCT);
}

std::vector<u32> AstQuery::get_enums() {
    return collect(AST_ENUM);
}

std::vector<u32> AstQuery::get_unions() {
    return collect(AST_UNION);
}

// the two kinds are collected in one pass and not in two, so that a 'const'
// written between two 'let' comes back between them: the order is the source's
std::vector<u32> AstQuery::get_global_variables() {
    return collect(AST_LET_DECLARATION, AST_CONST_DECLARATION);
}

// 'import a.b.*' is named "a.b": the star says where to look, not what to look
// for, so it is not a segment of the name
std::string AstQuery::get_import_name(u32 import) {
    std::string name;
    u32 path = find_child(import, AST_IMPORT_PATH);
    u32 node = ast->get_node(path)->get_children();

    while (node != 0) {
        AstNode* current = ast->get_node(node);

        if (current->get_kind() == AST_IMPORT_PATH_SEGMENT) {
            if (name.size() > 0) {
                name += '.';
            }

            name += text_of(node);
        }

        node = current->get_sibling();
    }

    return name;
}

bool AstQuery::is_star_import(u32 import) {
    u32 path = find_child(import, AST_IMPORT_PATH);

    return find_child(path, AST_IMPORT_ALL) != 0;
}

u32 AstQuery::get_import_offset(u32 import) {
    u32 segment = import_segment(import, false);

    return module->get_tokens()->get_token(
        ast->get_node(segment)->get_token()).get_offset();
}

// from the start of the first segment to the end of the last, so that the
// underline covers the dots too. The star is not part of it: it is not a
// segment, and an import that fails to resolve failed on the name
u32 AstQuery::get_import_length(u32 import) {
    Token last = module->get_tokens()->get_token(
        ast->get_node(import_segment(import, true))->get_token());

    return last.get_offset() + last.get_length() - get_import_offset(import);
}

std::string AstQuery::get_import_alias(u32 import) {
    u32 alias = find_child(import, AST_IMPORT_ALIAS);

    if (alias == 0) {
        return "";
    }

    return text_of(alias);
}

// the name hangs two levels down and not one: an AST_BINDING_NAME carries no
// token of its own, it wraps the AST_IDENTIFIER that does
std::string AstQuery::get_declaration_name(u32 declaration) {
    u32 name = find_child(declaration, AST_BINDING_NAME);
    u32 identifier = find_child(name, AST_IDENTIFIER);

    if (identifier == 0) {
        return "";
    }

    return text_of(identifier);
}

std::vector<u32> AstQuery::collect(AstNodeKind kind) {
    return collect(kind, kind);
}

std::vector<u32> AstQuery::collect(AstNodeKind first, AstNodeKind second) {
    std::vector<u32> found;
    u32 node = ast->get_node(ast->get_root())->get_children();

    while (node != 0) {
        AstNode* current = ast->get_node(node);
        AstNodeKind kind = current->get_kind();

        if (kind == first || kind == second) {
            found.push_back(node);
        }

        node = current->get_sibling();
    }

    return found;
}

// the first child of that kind, 0 when the parent has none. An optional part of
// a rule is simply absent from the tree, so 0 is an answer and not a failure
u32 AstQuery::find_child(u32 parent, AstNodeKind kind) {
    u32 node = ast->get_node(parent)->get_children();

    while (node != 0) {
        AstNode* current = ast->get_node(node);

        if (current->get_kind() == kind) {
            return node;
        }

        node = current->get_sibling();
    }

    return 0;
}

u32 AstQuery::import_segment(u32 import, bool last) {
    u32 path = find_child(import, AST_IMPORT_PATH);
    u32 node = ast->get_node(path)->get_children();
    u32 found = 0;

    while (node != 0) {
        AstNode* current = ast->get_node(node);

        if (current->get_kind() == AST_IMPORT_PATH_SEGMENT) {
            found = node;

            if (!last) {
                break;
            }
        }

        node = current->get_sibling();
    }

    return found;
}

std::string AstQuery::text_of(u32 node) {
    return std::string(module->get_token_value(ast->get_node(node)->get_token()));
}
