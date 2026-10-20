// Lays out every IR type of a project for one target, and asks g++ the same.
//
//   layout <case directory>            the IR's types and their layout
//   layout <case directory> --facts    sizes, alignments and offsets only
//   layout <case directory> --probe    the emitted C++, plus code that
//                                      prints those same facts as g++ laid
//                                      them out, and stops before main
//
// The target is read from the case's 'target' file, 'x86_64-linux' when
// there is none. The suite compares --facts with what --probe prints when
// built and run: Hadley chose the Itanium ABI's layout on 2026-10-20 so that
// g++ could check every one, and this is where it does.
#include <haard/compilation/compilation.h>
#include <haard/emitter/emitter.h>
#include <haard/ir/ir_class_table.h>
#include <haard/ir/ir_layout.h>
#include <haard/ir/ir_type_translator.h>

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace haard;

static Compilation compilation;
static IrTypeTable types;
static LayoutTable layouts;
static IrClassTable classes;

static AstNodeKind kind_of(const IrAggregate* aggregate) {
    return compilation.get_module(aggregate->module)
        ->get_ast()
        ->get_node(aggregate->declaration)
        ->get_kind();
}

// an enum's payload union is not a type C++ can name -- it is anonymous --
// so neither side states facts about it; its variants' structs say enough
static bool is_a_payload(IrTypeId type) {
    const IrAggregate* aggregate = types.get_aggregate(type);

    return types.get_type(type)->kind == IR_UNION
        && aggregate->declaration != 0 && kind_of(aggregate) == AST_ENUM;
}

static const char* kind_name(IrTypeId type) {
    switch (types.get_type(type)->kind) {
    case IR_CLASS:
        return "class";

    case IR_UNION:
        return "union";

    default:
        return "struct";
    }
}

static void dump(const Target& target) {
    std::cout << "target " << target.name << ": "
              << (target.big_endian ? "big" : "little")
              << " endian, pointers of " << target.pointer_size
              << " bytes\n";

    for (u32 i = 1; i < types.get_count(); i++) {
        IrTypeId type = {i};

        if (!types.is_aggregate(type)) {
            continue;
        }

        const IrAggregate* aggregate = types.get_aggregate(type);
        const IrLayout* layout = layouts.get_layout(type);

        std::cout << "\n" << kind_name(type) << " " << aggregate->name
                  << ": size " << layout->size << ", align " << layout->align
                  << ", data " << layout->data_size
                  << (aggregate->pod_for_layout ? ", pod" : "")
                  << (layout->empty ? ", empty" : "") << "\n";

        if (layout->dynamic) {
            std::cout << "    vptr at 0\n";
        }

        if (aggregate->base.index != 0) {
            std::cout << "    base " << types.name_of(aggregate->base)
                      << " at " << layout->base_offset << "\n";
        }

        for (u32 f = 0; f < aggregate->fields.size(); f++) {
            std::cout << "    " << aggregate->fields[f].name << " : "
                      << types.name_of(aggregate->fields[f].type) << " at "
                      << layout->offsets[f] << "\n";
        }
    }
}

// a method as a reader tells overloads apart: its name and its parameters,
// written as the source writes them
static std::string signature_of(const IrSlot& slot) {
    ExpressionTyper typer;
    Module* module = compilation.get_module(slot.module);
    std::vector<u32> parameters = module->get_types()->get_arguments(
        module->get_symbols()->get_candidate(slot.candidate)->type);
    std::string out;

    typer.set_compilation(&compilation);

    for (u32 i = 0; i + 1 < parameters.size(); i++) {
        out += (i > 0 ? ", " : "") + typer.name_in(slot.module, parameters[i]);
    }

    return slot.name + "(" + out + ")";
}

static void vtables() {
    for (u32 i = 1; i < types.get_count(); i++) {
        IrTypeId type = {i};
        const IrClass* entry = classes.get_class(type);

        if (entry == nullptr) {
            continue;
        }

        std::cout << "\nvtable of " << types.name_of(type) << ":\n"
                  << "    before 0: the descriptor\n";

        for (u32 s = 0; s < entry->slots.size(); s++) {
            const IrSlot& slot = entry->slots[s];

            std::cout << "    " << s << ": "
                      << (slot.candidate == 0 ? slot.name + ", implicit"
                                              : signature_of(slot) + ", by "
                                                    + types.name_of(
                                                        slot.implemented_by))
                      << "\n";
        }
    }
}

static void facts() {
    for (u32 i = 1; i < types.get_count(); i++) {
        IrTypeId type = {i};

        if (!types.is_aggregate(type) || is_a_payload(type)) {
            continue;
        }

        const IrAggregate* aggregate = types.get_aggregate(type);
        const IrLayout* layout = layouts.get_layout(type);

        std::cout << aggregate->name << " size " << layout->size << " align "
                  << layout->align << "\n";

        if (layout->dynamic) {
            std::cout << "  vptr 0\n";
        }

        if (aggregate->base.index != 0) {
            std::cout << "  base " << types.name_of(aggregate->base) << " "
                      << layout->base_offset << "\n";
        }

        for (u32 f = 0; f < aggregate->fields.size(); f++) {
            std::cout << "  " << aggregate->fields[f].name << " "
                      << layout->offsets[f] << "\n";
        }
    }
}

// The same facts, written as C++ for g++ to answer: one typedef per
// aggregate, then a function run before main that prints what sizeof,
// alignof and offsetof say about each, and leaves
static void probe(Emitter& emitter) {
    std::ostringstream names;
    std::ostringstream body;

    // the C++ name of each aggregate, and of an enum variant's struct, which
    // is the type of a member of the enum's anonymous union
    std::map<u32, std::string> spelled;

    body << "static int __haard_layout_probe() {\n";

    for (u32 i = 1; i < types.get_count(); i++) {
        IrTypeId type = {i};

        if (!types.is_aggregate(type)) {
            continue;
        }

        const IrAggregate* aggregate = types.get_aggregate(type);
        Module* module = compilation.get_module(aggregate->module);
        std::string written;

        if (aggregate->declaration == 0) {
            written = emitter.cpp_type(aggregate->module,
                                       aggregate->source_type);
        } else if (kind_of(aggregate) == AST_FIELD) {
            written = spelled[i];
        } else {
            written = emitter.cpp_name(
                aggregate->module,
                module->get_symbols()->candidate_of(aggregate->declaration));
        }

        // an enum's variants are named from here, while the enum is
        if (is_a_payload(type)) {
            u32 owner = 0;

            for (u32 o = 1; o < i; o++) {
                IrTypeId one = {o};

                if (types.is_aggregate(one)
                    && types.get_aggregate(one)->declaration
                           == aggregate->declaration
                    && types.get_aggregate(one)->module == aggregate->module
                    && !is_a_payload(one)) {
                    owner = o;
                }
            }

            for (const IrField& field : aggregate->fields) {
                spelled[field.type.index] =
                    "decltype(" + spelled[owner] + "::"
                    + emitter.cpp_name(aggregate->module, field.candidate)
                    + ")";
            }

            continue;
        }

        spelled[i] = written;

        std::string alias = "__haard_t" + std::to_string(i);
        const IrLayout* layout = layouts.get_layout(type);

        names << "typedef " << written << " " << alias << ";\n";
        body << "    std::printf(\"%s size %zu align %zu\\n\", \""
             << aggregate->name << "\", sizeof(" << alias << "), alignof("
             << alias << "));\n";

        if (layout->dynamic) {
            body << "    if (std::is_polymorphic<" << alias
                 << ">::value) {\n        std::printf(\"  vptr 0\\n\");\n"
                 << "    }\n";
        }

        if (aggregate->base.index != 0) {
            const IrAggregate* base = types.get_aggregate(aggregate->base);
            std::string above = emitter.cpp_name(
                base->module, compilation.get_module(base->module)
                                  ->get_symbols()
                                  ->candidate_of(base->declaration));

            body << "    std::printf(\"  base %s %zu\\n\", \"" << base->name
                 << "\", (size_t) ((char*) static_cast<" << above << "*>(("
                 << alias << "*) 4096) - (char*) 4096));\n";
        }

        for (u32 f = 0; f < aggregate->fields.size(); f++) {
            const IrField& field = aggregate->fields[f];
            std::string member;

            if (field.candidate != 0) {
                member = emitter.cpp_name(aggregate->module, field.candidate);
            } else if (field.name == "payload") {
                // the anonymous union is where its first variant is
                const IrAggregate* payload = types.get_aggregate(field.type);

                member = emitter.cpp_name(aggregate->module,
                                          payload->fields[0].candidate);
            } else {
                member = field.name;
            }

            body << "    std::printf(\"  %s %zu\\n\", \"" << field.name
                 << "\", offsetof(" << alias << ", " << member << "));\n";
        }
    }

    body << "    std::exit(0);\n    return 0;\n}\n\n"
         << "static int __haard_layout_probed = __haard_layout_probe();\n";

    std::cout << "\n// the layout probe of tests/ir_layout\n"
              << "#include <cstddef>\n#include <cstdlib>\n"
              << "#include <type_traits>\n\n"
              << names.str() << "\n" << body.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: layout <case directory> [--facts | --probe]\n";
        return 2;
    }

    std::filesystem::path directory = argv[1];
    std::string mode = argc > 2 ? argv[2] : "";
    std::filesystem::path table = directory / "table.tbl";
    std::string named = "x86_64-linux";
    Target target;

    if (std::filesystem::exists(directory / "target")) {
        std::ifstream file(directory / "target");

        std::getline(file, named);
    }

    if (!find_target(named, target)) {
        std::cout << "no target called '" << named << "'\n";
        return 1;
    }

    if (std::filesystem::exists(table) && !compilation.set_roots(table)) {
        std::cout << "table error: " << compilation.get_error() << '\n';
        return 1;
    }

    std::ifstream entry_file(directory / "entry");
    std::string entry;

    std::getline(entry_file, entry);

    if (!compilation.build(directory / entry)) {
        std::cout << "did not check:\n";
        compilation.print_diagnostics(std::cout);
        return 1;
    }

    IrTypeTranslator translator;

    translator.set_compilation(&compilation);
    translator.set_target(&target);
    translator.set_types(&types);
    layouts.set_target(&target);
    layouts.set_types(&types);

    if (!translator.translate_program()) {
        std::cout << "error: " << translator.get_error() << '\n';
        return 1;
    }

    if (!layouts.lay_out_all()) {
        std::cout << "error: " << layouts.get_error() << '\n';
        return 1;
    }

    if (mode == "--facts") {
        facts();
        return 0;
    }

    if (mode == "--probe") {
        Emitter emitter;

        emitter.set_compilation(&compilation);

        if (!emitter.emit(std::cout)) {
            std::cout << "cannot emit: " << emitter.get_error() << '\n';
            return 1;
        }

        probe(emitter);
        return 0;
    }

    dump(target);

    classes.set_compilation(&compilation);
    classes.set_types(&types);
    classes.build();
    vtables();

    return 0;
}
