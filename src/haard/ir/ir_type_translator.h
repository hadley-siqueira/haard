#ifndef HAARD_IR_TYPE_TRANSLATOR_H
#define HAARD_IR_TYPE_TRANSLATOR_H

#include <haard/ir/ir_type_table.h>
#include <haard/ir/target.h>
#include <haard/type_table/expression_typer.h>

#include <map>
#include <utility>
#include <vector>

namespace haard {
    // From the front end's types to the IR's (record 0077, stage 1).
    //
    // Every module has a type table of its own and an index means nothing
    // outside it (record 0016); the IR has one table for the program
    // (question 89). So a declaration is interned by the module and the node
    // that declare it, and a struct reached from two modules is one IR type.
    //
    // What each Haard type becomes:
    //
    //   - a whole number, a char, a bool, a float: its width (question 79);
    //     'isize' and 'usize' the target's pointer width
    //   - a symbol, a pointer, a 'T&' and a 'T&&': 'ptr' (question 80)
    //   - 'T[n]': an array; 'T[]' has no layout here, the sugar pass has
    //     made it an 'Array<T>' wherever a value of it is held
    //   - a class, a struct, a union: an aggregate of its own, with its base
    //   - an enum with no payload: its tag, the smallest unsigned width that
    //     holds the largest (Hadley, 2026-10-20). With one, or with a method:
    //     a struct of the tag and a union of one struct per variant that
    //     carries something, which is the C++ the emitter writes
    //   - a tuple: a struct per shape, by its IR element types (record 0067)
    //   - a function value: two pointers, the environment and the function,
    //     in that order (record 0058)
    //   - a generic parameter, and 'void': no type
    class IrTypeTranslator {
        public:
            IrTypeTranslator();

        public:
            void set_compilation(Compilation* compilation);
            void set_target(const Target* target);
            void set_types(IrTypeTable* types);

            // every type declaration of the program that is not a generic,
            // and every tuple in any module's table. False when one of them
            // holds something with no layout, with the reason in get_error
            bool translate_program();

            // index 0 for a type with none
            IrTypeId translate(u32 module, u32 type);

            const std::string& get_error();

        private:
            IrTypeId of_declaration(u32 module, u32 declaration);
            IrTypeId of_enum(u32 module, u32 declaration,
                             const std::string& name);
            IrTypeId of_tuple(u32 module, u32 type);
            IrTypeId of_function(u32 module, u32 type);

            // the fields of a struct, a class or a union, in source order
            std::vector<IrField> fields_of(u32 module, u32 declaration,
                                           const std::string& owner);

            bool is_pod_for_layout(u32 module, u32 declaration);
            bool is_generic(u32 module, u32 declaration);
            bool is_concrete(u32 module, u32 type);

            // 'Point', 'geometry.Point', 'Pair<i32, u8>'
            std::string name_of(u32 module, u32 declaration);

            IrTypeId nothing();

        private:
            Compilation* compilation;
            const Target* target;
            IrTypeTable* types;

            // for the name of an instantiation's arguments, written as the
            // source writes them
            ExpressionTyper typer;

            // (module, declaration) to the IR type
            std::map<std::pair<u32, u32>, u32> declarations;

            // a tuple's IR element types to its struct
            std::map<std::vector<u32>, u32> tuples;

            // the one struct every function value is, 0 until one is met
            u32 function_value;

            std::string error;
    };
}

#endif
