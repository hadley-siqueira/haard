#include <haard/compilation/compilation.h>
#include <haard/sugar/switch_lowerer.h>

using namespace haard;

SwitchLowerer::SwitchLowerer() {
    compilation = nullptr;
    module = nullptr;
    index = 0;
    like = 0;
    counter = 0;
    worked = false;
}

void SwitchLowerer::set_compilation(Compilation* compilation) {
    this->compilation = compilation;

    typer.set_compilation(compilation);
}

void SwitchLowerer::set_collector(TypeCollector* collector) {
    typer.set_collector(collector);
}

bool SwitchLowerer::lower(u32 index) {
    this->index = index;
    module = compilation->get_module(index);
    worked = false;

    builder.set_ast(module->get_ast());
    query.set_module(module);
    symbols.set_module(module);
    typer.set_module(index);

    walk(module->get_ast()->get_root(), module->get_symbols()
                                            ->get_module_scope());

    return worked;
}

// The tree, looking for a switch and carrying the scope every name in it is
// resolved against -- the same walk shape the checkers have, with the scope
// picked up from the node that opened it
void SwitchLowerer::walk(u32 node, u32 scope) {
    if (node == 0) {
        return;
    }

    SymbolTable* table = module->get_symbols();
    u32 opened = table->scope_owned_by(node);

    if (opened != 0) {
        scope = opened;
    }

    // read the children before the walk: rewriting a switch replaces the node
    // it was given, and what it holds afterwards is not what it held before
    std::vector<u32> children;

    for (u32 child = first_child(node); child != 0; child = sibling_of(child)) {
        children.push_back(child);
    }

    if (kind_of(node) == AST_SWITCH) {
        u32 given = typer.type_of(index, scope, first_child(node),
                                  INVALID_TYPE);

        if (given != INVALID_TYPE && !switchable(given)) {
            rewrite(node, scope, given);
        }
    }

    for (u32 child : children) {
        walk(child, scope);
    }
}

bool SwitchLowerer::switchable(u32 given) {
    TypeTable* types = module->get_types();
    Type* entry = types->get_type(types->value_of(given));

    // an enum is a tag and a tag is an integer, whether or not it is written
    // as one
    if (entry->kind == TYPE_NAMED) {
        Module* holder = compilation->get_module(entry->module);

        return (SymbolKind) holder->get_symbols()
                   ->get_candidate(entry->subject)->kind == SYMBOL_ENUM;
    }

    if (entry->kind != TYPE_BUILTIN) {
        return false;
    }

    switch ((BuiltinType) entry->subject) {
    case BUILTIN_BOOL:
    case BUILTIN_VOID:
    case BUILTIN_F32:
    case BUILTIN_F64:
        return false;

    default:
        break;
    }

    return true;
}

// The switch becomes a **block** holding the subject's local and the chain,
// rewritten in place the way record 0032's template string and record 0040's
// foreach are: it keeps its index and its sibling, so nothing else in the tree
// moves and no walk has to know a parent
void SwitchLowerer::rewrite(u32 node, u32 scope, u32 given) {
    Ast* ast = module->get_ast();
    u32 subject = first_child(node);

    like = token_of(node);

    std::string named = "__sw" + std::to_string(counter++);
    u32 name = module->add_synthetic_token(TK_IDENTIFIER, named, like);

    // the cases are read off the chain **before** the subject leaves it: the
    // subject is the first child, and cutting its sibling link is what takes
    // the rest of the list with it
    std::vector<u32> cases;

    for (u32 child = sibling_of(subject); child != 0;
         child = sibling_of(child)) {
        cases.push_back(child);
    }

    // and then it moves into the local, so what stands in the conditions is
    // the name
    ast->get_node(subject)->set_sibling(0);

    // a **reference** when the subject has a name to refer to, so a chain of
    // comparisons over a String copies nothing -- and a copy when it does
    // not, since a temporary has to outlive the chain
    u32 held = is_an_rvalue(subject)
                   ? given
                   : module->get_types()->reference(
                         module->get_types()->value_of(given));

    u32 local = make_local(scope, name, subject, held);

    // every case, in order, with the ones that share a block joined by 'or'
    std::vector<u32> written;
    u32 first = 0;
    u32 chain = 0;

    for (u32 child : cases) {
        if (first == 0) {
            first = child;
        }

        u32 block = kind_of(child) == AST_DEFAULT
                        ? first_child(child)
                        : sibling_of(first_child(child));

        // a case with no block runs the block of the one below it, so it is
        // held until that one arrives
        if (block == 0) {
            continue;
        }

        u32 branch;

        if (kind_of(child) == AST_DEFAULT) {
            branch = builder.make_else(
                module->add_synthetic_token(TK_ELSE, "else", like));

            builder.add_child(branch, 0, block);
        } else {
            bool first_branch = chain == 0;
            u32 token = module->add_synthetic_token(
                first_branch ? TK_IF : TK_ELIF,
                first_branch ? "if" : "elif", like);

            branch = first_branch ? builder.make_if(token)
                                  : builder.make_elif(token);

            u32 last = builder.add_child(
                branch, 0, condition_of(name, child, first, child));

            builder.add_child(branch, last, block);
        }

        if (chain == 0) {
            chain = branch;
        } else {
            written.push_back(branch);
        }

        first = 0;
    }

    // the elifs and the else are children of the 'if', which is the shape the
    // parser builds and the printer and the emitter both read
    u32 last = 0;

    for (u32 branch : written) {
        last = builder.add_child(chain, last, branch);
    }

    AstNode* rewritten = ast->get_node(node);

    rewritten->set_kind(AST_BLOCK);
    rewritten->set_children(0);

    u32 tail = builder.add_child(node, 0, local);

    builder.add_child(node, tail, chain);

    worked = true;
}

// '__sw0 == <pattern>', and the patterns of every case grouped with this one
// joined by 'or' -- which is what sharing a block means when there is no
// switch to fall through
u32 SwitchLowerer::condition_of(u32 name, u32 one_case, u32 from, u32 to) {
    Ast* ast = module->get_ast();
    u32 whole = 0;

    for (u32 child = from; child != 0; child = sibling_of(child)) {
        u32 equal = module->add_synthetic_token(TK_EQUAL, "==", like);
        u32 one = builder.make_binary_operator(
            AST_EQUAL, equal, builder.make_identifier(name),
            first_child(child));

        // the pattern leaves the case and becomes the right side, so what it
        // used to be followed by there is the block -- which this branch has
        // already taken
        ast->get_node(one)->set_sibling(0);

        if (whole == 0) {
            whole = one;
        } else {
            u32 word = module->add_synthetic_token(TK_OR, "or", like);


            whole = builder.make_binary_operator(AST_LOGICAL_OR, word, whole,
                                                 one);
        }

        if (child == to) {
            break;
        }
    }

    (void) one_case;

    return whole;
}

// 'let __swN = <subject>', declared in the scope the switch was written in and
// given the subject's own type. Record 0039's two calls, which is what a
// declaration made after the symbol phase needs
u32 SwitchLowerer::make_local(u32 scope, u32 name, u32 subject, u32 type) {
    u32 token = module->add_synthetic_token(TK_LET, "let", like);
    u32 binding = builder.make_binding(
        builder.make_binding_name(builder.make_identifier(name)), 0,
        builder.make_binding_expression(subject));
    u32 statement = builder.make_let_declaration(token, binding);

    symbols.collect_declaration(scope, statement, "");

    u32 candidate = module->get_symbols()->candidate_of(binding);

    if (candidate != 0) {
        module->get_symbols()->set_candidate_type(candidate, type);
    }

    return statement;
}

// whether this expression gives back a value with no name, which is the one
// thing a reference cannot be bound to. The emitter asks the same question of
// the same shapes
bool SwitchLowerer::is_an_rvalue(u32 node) {
    switch (kind_of(node)) {
    case AST_CALL:
    case AST_LIST:
    case AST_ARRAY:
        return true;

    case AST_PARENTHESIS:
        return is_an_rvalue(first_child(node));

    default:
        break;
    }

    return false;
}

AstNodeKind SwitchLowerer::kind_of(u32 node) {
    return module->get_ast()->get_node(node)->get_kind();
}

u32 SwitchLowerer::first_child(u32 node) {
    return module->get_ast()->get_node(node)->get_children();
}

u32 SwitchLowerer::sibling_of(u32 node) {
    return module->get_ast()->get_node(node)->get_sibling();
}

u32 SwitchLowerer::token_of(u32 node) {
    return module->get_ast()->get_node(node)->get_token();
}
