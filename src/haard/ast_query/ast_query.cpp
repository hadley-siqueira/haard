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

std::vector<u32> AstQuery::get_declarations() {
    std::vector<u32> declarations;

    for (u32 node = ast->get_node(ast->get_root())->get_children(); node != 0;
         node = ast->get_node(node)->get_sibling()) {
        switch (ast->get_node(node)->get_kind()) {
        case AST_FUNCTION:
        case AST_CLASS:
        case AST_STRUCT:
        case AST_ENUM:
        case AST_UNION:
        case AST_LET_DECLARATION:
        case AST_CONST_DECLARATION:
            declarations.push_back(node);
            break;

        // an import declares nothing of its own: what it brings in is bound
        // by record 0008 and is not a declaration of this file
        default:
            break;
        }
    }

    return declarations;
}

std::vector<u32> AstQuery::get_members(u32 declaration) {
    std::vector<u32> members;
    u32 body = find_child(declaration, AST_TYPE_BODY);

    if (body == 0) {
        return members;
    }

    for (u32 member = ast->get_node(body)->get_children(); member != 0;
         member = ast->get_node(member)->get_sibling()) {
        // 'pass' fills an empty body and declares nothing
        if (ast->get_node(member)->get_kind() != AST_PASS) {
            members.push_back(member);
        }
    }

    return members;
}

std::vector<u32> AstQuery::get_params(u32 function) {
    std::vector<u32> params;

    for (u32 child = ast->get_node(function)->get_children(); child != 0;
         child = ast->get_node(child)->get_sibling()) {
        if (ast->get_node(child)->get_kind() == AST_PARAM) {
            params.push_back(child);
        }
    }

    return params;
}

std::vector<u32> AstQuery::get_generic_parameters(u32 declaration) {
    std::vector<u32> parameters;
    u32 list = find_child(declaration, AST_GENERIC_PARAMETERS);

    if (list == 0) {
        return parameters;
    }

    for (u32 child = ast->get_node(list)->get_children(); child != 0;
         child = ast->get_node(child)->get_sibling()) {
        parameters.push_back(child);
    }

    return parameters;
}

u32 AstQuery::get_written_type(u32 declaration) {
    u32 wrapper = find_child(declaration, AST_BINDING_TYPE);

    if (wrapper == 0) {
        wrapper = find_child(declaration, AST_FUNCTION_RETURN_TYPE);
    }

    return wrapper == 0 ? 0 : ast->get_node(wrapper)->get_children();
}

u32 AstQuery::get_super_type(u32 declaration) {
    u32 wrapper = find_child(declaration, AST_SUPER_TYPE);

    return wrapper == 0 ? 0 : ast->get_node(wrapper)->get_children();
}

u32 AstQuery::get_binding_expression(u32 declaration) {
    // an assignment that declares by being written: 'let' is not required, so
    // 'n = 1' with no n in view is a declaration whose value is the right side
    if (ast->get_node(declaration)->get_kind() == AST_ASSIGNMENT) {
        u32 target = ast->get_node(declaration)->get_children();

        return target == 0 ? 0 : ast->get_node(target)->get_sibling();
    }

    u32 wrapper = find_child(declaration, AST_BINDING_EXPRESSION);

    return wrapper == 0 ? 0 : ast->get_node(wrapper)->get_children();
}

std::vector<u32> AstQuery::get_loop_variables(u32 for_each) {
    std::vector<u32> variables;
    u32 head = find_child(for_each, AST_FOR_HEAD);

    if (head == 0) {
        return variables;
    }

    for (u32 child = ast->get_node(head)->get_children(); child != 0;
         child = ast->get_node(child)->get_sibling()) {
        AstNodeKind kind = ast->get_node(child)->get_kind();
        u32 name = child;

        // the last expression of the head is the 'in', and what it binds is
        // its left side. Everything before it binds itself
        if (kind == AST_IN || kind == AST_NOT_IN) {
            name = ast->get_node(child)->get_children();
        }

        if (name != 0 && ast->get_node(name)->get_kind() == AST_IDENTIFIER) {
            variables.push_back(name);
        }
    }

    return variables;
}

u32 AstQuery::get_block(u32 node) {
    return find_child(node, AST_BLOCK);
}

std::vector<u32> AstQuery::get_children(u32 node) {
    std::vector<u32> children;

    for (u32 child = ast->get_node(node)->get_children(); child != 0;
         child = ast->get_node(child)->get_sibling()) {
        children.push_back(child);
    }

    return children;
}

u32 AstQuery::get_binding(u32 statement) {
    return find_child(statement, AST_BINDING);
}

// A binding names one thing, or several when a tuple target takes a value
// apart. Both go under one AST_BINDING_NAME and the difference is its child:
// an AST_IDENTIFIER for 'let a', an AST_TUPLE whose children are the
// identifiers for 'let (a, b)'. Parser::parse_binding_target is where the
// bracket decides, and it wraps the tuple in a binding name like any other
std::vector<std::string> AstQuery::get_binding_names(u32 statement) {
    std::vector<std::string> names;
    u32 name = find_child(get_binding(statement), AST_BINDING_NAME);
    u32 target = ast->get_node(name)->get_children();

    if (target == 0) {
        return names;
    }

    if (ast->get_node(target)->get_kind() != AST_TUPLE) {
        names.push_back(text_of(target));

        return names;
    }

    for (u32 child = ast->get_node(target)->get_children(); child != 0;
         child = ast->get_node(child)->get_sibling()) {
        names.push_back(text_of(child));
    }

    return names;
}

// the name hangs two levels down and not one: an AST_BINDING_NAME carries no
// token of its own, it wraps the AST_IDENTIFIER that does
std::string AstQuery::get_declaration_name(u32 declaration) {
    // an assignment that declares by being written names what is on its left,
    // which is one identifier or it would not have declared anything
    if (ast->get_node(declaration)->get_kind() == AST_ASSIGNMENT) {
        u32 target = ast->get_node(declaration)->get_children();

        return target == 0
            || ast->get_node(target)->get_kind() != AST_IDENTIFIER
                   ? ""
                   : text_of(target);
    }

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
