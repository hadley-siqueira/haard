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
            void collect(u32 module);
            void infer(u32 module);

        private:
            // 'given' is false in the first pass, where a binding keeps only
            // the type it wrote, and true in the second
            u32 type_of(u32 candidate, u32 scope, bool given);

            void walk(u32 index, bool given);

            // a 'def': the parameter types in source order and the return type
            // last, per record 0016. A missing return type is void, which is
            // what a function that states none returns
            u32 signature_of(u32 node, u32 scope);

            // a field, a parameter, a binding: what it wrote, or what it was
            // given when it wrote nothing
            u32 written_or_inferred(u32 node, u32 scope);

            // the type a class, a struct or a union derives from, and
            // INVALID_TYPE for everything else
            u32 super_of(u32 candidate, u32 scope);

        private:
            Compilation* compilation;
            TypeBuilder builder;
            ExpressionTyper typer;

            Module* module;
            u32 index;

            std::map<u32, u32> scope_of;
    };
}

#endif
