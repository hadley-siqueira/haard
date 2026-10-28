#include <haard/lowering/ir_lowerer.h>

#include <haard/ir/ir_flattener.h>
#include <haard/ir/ir_verifier.h>
#include <haard/lowering/function_lowerer.h>

using namespace haard;

IrLowerer::IrLowerer() {
    compilation = nullptr;
    target = nullptr;
    program = nullptr;
    alloc_native.index = 0;
    free_native.index = 0;
}

void IrLowerer::set_compilation(Compilation* compilation) {
    this->compilation = compilation;
}

void IrLowerer::set_target(const Target* target) {
    this->target = target;
}

void IrLowerer::set_program(IrProgram* program) {
    this->program = program;
}

bool IrLowerer::lower() {
    error.clear();
    translator.set_compilation(compilation);
    translator.set_target(target);
    translator.set_types(program->get_types());

    return declare_functions() && declare_globals() && lower_bodies()
        && lower_globals() && verify("lowering");
}

bool IrLowerer::finish() {
    IrFlattener flattener;

    flattener.set_program(program);

    for (u32 i = 1; i < program->get_object_count(); i++) {
        IrObject& object = program->get_object({i});

        if (object.kind == IR_OBJECT_FUNCTION && !object.native) {
            flattener.flatten({i});
        }
    }

    return verify("flatten");
}

const std::string& IrLowerer::get_error() {
    return error;
}

Compilation* IrLowerer::get_compilation() {
    return compilation;
}

IrTypeTranslator* IrLowerer::get_translator() {
    return &translator;
}

const Target* IrLowerer::get_target() {
    return target;
}

IrObjectId IrLowerer::global_of(u32 module, u32 candidate) {
    std::map<std::pair<u32, u32>, u32>::iterator found =
        globals.find(std::make_pair(module, candidate));
    IrObjectId id = {found == globals.end() ? 0 : found->second};

    return id;
}

IrObjectId IrLowerer::text(const std::string& bytes) {
    std::map<std::string, u32>::iterator found = texts.find(bytes);

    if (found != texts.end()) {
        return {found->second};
    }

    IrObjectId made =
        program->add_text("text." + std::to_string(texts.size()), bytes);

    texts[bytes] = made.index;

    return made;
}

IrObjectId IrLowerer::allocator() {
    if (alloc_native.index == 0) {
        IrTypeTable* types = program->get_types();

        alloc_native = program->add_native(
            "__alloc",
            program->signature({types->integer(target->pointer_size)},
                               types->pointer(IR_SPACE_GENERIC)));
    }

    return alloc_native;
}

IrObjectId IrLowerer::deallocator() {
    if (free_native.index == 0) {
        free_native = program->add_native(
            "__free",
            program->signature(
                {program->get_types()->pointer(IR_SPACE_GENERIC)}, {0}));
    }

    return free_native;
}

IrObjectId IrLowerer::function_of(u32 module, u32 candidate) {
    std::map<std::pair<u32, u32>, u32>::iterator found =
        functions.find(std::make_pair(module, candidate));
    IrObjectId id = {found == functions.end() ? 0 : found->second};

    return id;
}

IrTypeId IrLowerer::type_of(u32 module, u32 type) {
    return translator.translate(module, type);
}

void IrLowerer::fail(const std::string& message) {
    if (error.size() == 0) {
        error = message;
    }
}

// An object for every function first, so a call can name one declared
// further down or in another module. A function of 'std.low_io' is a native
// (record 0030), and the entry module's 'main' is the one called 'main'
bool IrLowerer::declare_functions() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        SymbolTable* symbols = module->get_symbols();
        AstQuery query;

        query.set_module(module);

        for (u32 node : query.get_functions()) {
            u32 candidate = symbols->candidate_of(node);

            if (candidate == 0 || is_generic(i, node)) {
                continue;
            }

            std::vector<u32> written = module->get_types()->get_arguments(
                symbols->get_candidate(candidate)->type);
            std::vector<IrTypeId> parameters;

            for (u32 p = 0; p + 1 < written.size(); p++) {
                IrTypeId one = type_of(i, written[p]);

                if (one.index == 0 || program->get_types()->is_aggregate(one)) {
                    fail("not lowered yet: a parameter of a type that is not "
                         "a scalar");
                    return false;
                }

                parameters.push_back(one);
            }

            IrTypeId result = written.size() == 0 ? IrTypeId{0}
                                                  : type_of(i, written.back());

            if (program->get_types()->is_aggregate(result)) {
                fail("not lowered yet: a function giving back an aggregate");
                return false;
            }

            std::string name = query.get_declaration_name(node);

            if (!(i == 0 && name == "main")) {
                name = module->get_name().size() > 0
                           ? module->get_name() + "." + name
                           : name;
            }

            name = unique(name);

            u32 signature = program->signature(parameters, result);
            IrObjectId made =
                module->get_name() == "std.low_io"
                    ? program->add_native(query.get_declaration_name(node),
                                          signature)
                    : program->add_function(name, signature,
                                            IR_FORM_STRUCTURED);

            functions[std::make_pair(i, candidate)] = made.index;

            if (!program->get_object(made).native) {
                bodies.push_back(std::make_pair(i, node));
            }
        }
    }

    return true;
}

// Record 0026's rule holds for a global too: a type that is not a scalar or
// an array of them waits for step 3, with its 'init'
bool IrLowerer::declare_globals() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        SymbolTable* symbols = module->get_symbols();
        AstQuery query;

        query.set_module(module);

        for (u32 statement : query.get_global_variables()) {
            for (u32 binding = module->get_ast()->get_node(statement)
                                   ->get_children();
                 binding != 0;
                 binding = module->get_ast()->get_node(binding)->get_sibling()) {
                u32 candidate = symbols->candidate_of(binding);

                if (candidate == 0) {
                    fail("not lowered yet: a global that takes a value apart");
                    return false;
                }

                IrTypeId type =
                    type_of(i, symbols->get_candidate(candidate)->type);

                if (type.index == 0 || program->get_types()->is_aggregate(type)) {
                    fail("not lowered yet: a global of a type that is not a "
                         "scalar or an array");
                    return false;
                }

                // the binding's name is the identifier under its
                // AST_BINDING_NAME
                std::string name;

                for (u32 part = module->get_ast()->get_node(binding)
                                    ->get_children();
                     part != 0;
                     part = module->get_ast()->get_node(part)->get_sibling()) {
                    AstNode* node = module->get_ast()->get_node(part);

                    if (node->get_kind() == AST_BINDING_NAME
                        && node->get_children() != 0) {
                        name = std::string(module->get_token_value(
                            module->get_ast()
                                ->get_node(node->get_children())
                                ->get_token()));
                    }
                }

                if (module->get_name().size() > 0) {
                    name = module->get_name() + "." + name;
                }

                globals[std::make_pair(i, candidate)] =
                    program->add_zeroed(unique(name), type).index;
            }
        }
    }

    return true;
}

bool IrLowerer::lower_bodies() {
    for (const std::pair<u32, u32>& one : bodies) {
        Module* module = compilation->get_module(one.first);
        u32 candidate = module->get_symbols()->candidate_of(one.second);
        FunctionLowerer body(this, program);

        if (!body.lower(one.first, one.second,
                        function_of(one.first, candidate))) {
            return false;
        }
    }

    return true;
}

bool IrLowerer::lower_globals() {
    for (u32 i = 0; i < compilation->get_module_count(); i++) {
        Module* module = compilation->get_module(i);
        AstQuery query;

        query.set_module(module);

        if (query.get_global_variables().size() == 0) {
            continue;
        }

        std::string name = module->get_name().size() > 0
                               ? module->get_name() + ".globals"
                               : "globals";
        IrObjectId made = program->add_function(
            unique(name), program->signature({}, {0}), IR_FORM_STRUCTURED);
        FunctionLowerer body(this, program);

        program->add_initialiser(made);

        if (!body.lower_globals(i, made)) {
            return false;
        }
    }

    return true;
}

std::string IrLowerer::unique(const std::string& name) {
    u32 taken = names[name]++;

    return taken > 0 ? name + "#" + std::to_string(taken) : name;
}

// question 88: always, after the lowering and after flatten. A failure here
// is the compiler's bug, never the program's
bool IrLowerer::verify(const std::string& when) {
    IrVerifier verifier;

    verifier.set_program(program);

    if (verifier.verify()) {
        return true;
    }

    fail("the IR does not verify after " + when + ": "
         + verifier.get_errors()[0].message);

    return false;
}

bool IrLowerer::is_generic(u32 module_index, u32 declaration) {
    Module* module = compilation->get_module(module_index);
    AstQuery query;

    query.set_module(module);

    if (query.get_generic_parameters(declaration).size() == 0) {
        return false;
    }

    return module->get_instantiation(
               module->get_symbols()->candidate_of(declaration))
        == nullptr;
}
