#ifndef HAARD_TYPE_COLLECTOR_H
#define HAARD_TYPE_COLLECTOR_H

#include <haard/type_table/expression_typer.h>
#include <map>

namespace haard {
    // Gives every declaration of a module its type. The symbol side of the
    // family says what names exist; this says what each of them is.
    //
    // It runs over the symbol table and not over the tree, because that is
    // where the declarations already are: every scope, every name, every
    // candidate. What a candidate is comes from its kind, and where its type
    // names resolve comes from the scope it sits in -- a field's type is
    // written inside the class, so a generic parameter of that class is in
    // view exactly where the field's candidate lives.
    //
    // The one that is not in the scope it needs is a function: its candidate
    // sits in the module or the class, while its parameters and its own
    // generic parameters live in the scope it opened. So this reads
    // Scope::owner backwards for the same reason the UseResolver does.
    //
    // A 'let' with no written type gets the type of what it was given, which
    // is record 0018's inference: 'let a = 1' is an i32 because an integer
    // literal with nothing expected is one. A 'let' that writes a type instead
    // hands it down, so the literal in 'let n: i64 = 1' is an i64 and never
    // converted.
    class TypeCollector {
        public:
            TypeCollector();

        public:
            void set_compilation(Compilation* compilation);

            // Two passes, and they cannot be one. The first gives every
            // declaration the type it **wrote** -- signatures, fields,
            // parameters, bases. The second infers what a binding was
            // **given**, and that needs every module's declarations already
            // typed, because 'let p = make(1)' resolves a call whose candidate
            // may live in a module the first pass had not reached.
            //
            // It is the same reason the use pass is a walk of its own: a phase
            // that reads across modules cannot run inside the walk that fills
            // them.
            //
            // Each gives back whether it typed anything, because record
            // 0002's instantiation clones a declaration into the module that
            // **declared** the generic -- which may be a module this walk has
            // already passed. So the caller goes round again until a round
            // types nothing
            bool collect(u32 module);
            bool infer(u32 module);

        private:
            // 'given' is false in the first pass, where a binding keeps only
            // the type it wrote, and true in the second
            u32 type_of(u32 candidate, u32 scope, bool given);

            bool walk(u32 index, bool given);

            // a 'def': the parameter types in source order and the return type
            // last, per record 0016. A missing return type is void, which is
            // what a function that states none returns
            u32 signature_of(u32 node, u32 scope);

            // a field, a parameter, a binding: what it wrote, or what it was
            // given when it wrote nothing. 'written' is what the first pass
            // built and is handed in rather than built again -- rebuilding it
            // would ask the same questions of the same tree and report every
            // answer twice
            u32 written_or_inferred(u32 node, u32 scope, u32 written);

            // the type a class, a struct or a union derives from, and
            // INVALID_TYPE for everything else
            u32 super_of(u32 candidate, u32 scope);

            // Record 0026 runs a class's own 'init' with no arguments wherever
            // a value of it comes into being with none written: a local, a
            // global, a field held by value, and the base of a class being
            // built. A class whose every 'init' needs an argument cannot do
            // that, and there is no syntax yet for writing a base's arguments.
            //
            // A class that wrote **no** 'init' is not that case and never was:
            // C++ builds it for nothing, and its implicit constructor runs the
            // constructors of its class-typed fields down the whole chain --
            // checked by running one, 2026-09-05. So there is nothing to
            // generate, and the only thing 5.5 was ever about is **where the
            // question is asked**.
            //
            // It was asked in the emitter, which meant 'hdc file.hd' said the
            // program was fine and 'hdc --emit-cpp file.hd' said it was not,
            // and the message it gave had no file, no line and no caret --
            // alone among this compiler's diagnostics. It belongs here, where
            // a declaration gets its type, which is also the one place that
            // holds the node to point at
            void require_default_construction(u32 candidate);

            // whether a value of this type can come into being with no
            // arguments written. True for everything that is not a class
            bool builds_with_nothing(u32 type);

            // the identifier a declaration was named with, which is where a
            // diagnostic about the declaration points. NOT the written type:
            // a composite type node carries no token of its own and reporting
            // at one lands on the first token of the file
            u32 name_node_of(u32 declaration);

            void report(u32 node, const std::string& message);

        private:
            Compilation* compilation;
            TypeBuilder builder;
            ExpressionTyper typer;
            Coercion coercion;

            Module* module;
            u32 index;

            std::map<u32, u32> scope_of;

            // per module, the last candidate each pass has typed. A round
            // takes only what is past the mark, so re-entering a module that
            // grew costs the new declarations and reports nothing twice
            std::map<u32, u32> collected;
            std::map<u32, u32> inferred;
    };
}

#endif
