#include <haard/ast/ast_builder.h>
#include <haard/compilation/compilation.h>
#include <haard/symbol_table/symbol_collector.h>
#include <haard/type_table/instantiator.h>

using namespace haard;

// Deep enough that no program written by hand reaches it, shallow enough that
// one written by mistake stops in under a second
static const u32 NESTING_LIMIT = 16;

Instantiator::Instantiator() {
    compilation = nullptr;
}

void Instantiator::set_compilation(Compilation* compilation) {
    this->compilation = compilation;
}

u32 Instantiator::instantiate(u32 caller, u32 node, u32 owner, u32 candidate,
                              const std::vector<u32>& arguments) {
    Module* host = compilation->get_module(owner);
    SymbolTable* table = host->get_symbols();
    u32 declaration = table->get_candidate(candidate)->ast_node;
    AstQuery query;

    query.set_module(host);

    std::vector<u32> parameters = query.get_generic_parameters(declaration);

    if (parameters.size() != arguments.size()) {
        report(caller, node, "'" + query.get_declaration_name(declaration) +
               "' takes " + std::to_string(parameters.size()) +
               " type argument" + (parameters.size() == 1 ? "" : "s") +
               ", and " + std::to_string(arguments.size()) +
               (arguments.size() == 1 ? " was" : " were") + " written");

        return 0;
    }

    for (u32 argument : arguments) {
        if (depth_of(owner, argument) >= NESTING_LIMIT) {
            report(caller, node, "this instantiation nests more than " +
                   std::to_string(NESTING_LIMIT) + " deep");

            return 0;
        }
    }

    // Before anything is built, and that is what makes 'class Node<T>: next :
    // Node<T>*' stop: the clone's own fields are typed later, by the walk, and
    // by then this answers
    u32 already = host->find_instantiation(candidate, arguments);

    if (already != 0) {
        return already;
    }

    AstBuilder builder;
    SymbolCollector collector;
    u32 root = host->get_ast()->get_root();
    std::string name = query.get_declaration_name(declaration);

    // the arguments by their index in this module's table, which is the one
    // thing that is both unique and unwritable
    for (u32 argument : arguments) {
        name += "#" + std::to_string(argument);
    }

    builder.set_ast(host->get_ast());

    u32 copy = builder.clone(declaration);

    // a declaration is a child of the root and never deeper, which is what
    // every walk after this one assumes
    builder.add_child(root, 0, copy);

    collector.set_module(host);
    collector.collect_declaration(table->get_module_scope(), copy, name);

    u32 made = table->candidate_of(copy);

    if (made == 0) {
        return 0;
    }

    // written before the parameters are bound, so that a body reaching this
    // same instantiation finds it however deep it went
    host->add_instantiation(candidate, made, arguments);

    // and this is the whole of the substitution: inside the clone, the
    // parameter is not a stand-in for a type, it is that type
    std::vector<u32> cloned = query.get_generic_parameters(copy);

    for (u32 i = 0; i < cloned.size() && i < arguments.size(); i++) {
        u32 parameter = table->candidate_of(cloned[i]);

        if (parameter != 0) {
            table->set_candidate_type(parameter, arguments[i]);
        }
    }

    return made;
}

u32 Instantiator::depth_of(u32 owner, u32 type) {
    Module* host = compilation->get_module(owner);
    TypeTable* types = host->get_types();
    Type* entry = types->get_type(type);
    u32 deepest = 0;

    // An instantiation carries no arguments in its type -- it IS a class, and
    // that is the point of cloning -- so the nesting cannot be read off the
    // type and has to come from the record that made it. Without this the
    // guard measures nothing: every argument reaching here is already flat
    if (entry->kind == TYPE_NAMED) {
        Module* declaring = compilation->get_module(entry->module);
        const Instantiation* made = declaring->get_instantiation(entry->subject);

        if (made == nullptr) {
            return 0;
        }

        for (u32 argument : made->arguments) {
            u32 depth = depth_of(entry->module, argument);

            if (depth > deepest) {
                deepest = depth;
            }
        }

        return deepest + 1;
    }

    for (u32 argument : types->get_arguments(type)) {
        u32 depth = depth_of(owner, argument);

        if (depth > deepest) {
            deepest = depth;
        }
    }

    return deepest;
}

void Instantiator::report(u32 caller, u32 node, const std::string& message) {
    Module* module = compilation->get_module(caller);
    Token& token = module->get_tokens()->get_token(
        module->get_ast()->get_node(node)->get_token());

    module->get_logger()->error(token.get_offset(), token.get_length(),
                                message);
}
