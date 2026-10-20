#ifndef HAARD_IR_LOWERER_H
#define HAARD_IR_LOWERER_H

#include <haard/ir/ir_program.h>
#include <haard/ir/ir_type_translator.h>

#include <map>
#include <utility>

namespace haard {
    // From a checked program to the IR (record 0077, stage 4).
    //
    // It reads what the C++ emitter reads -- the Ast after the sugar passes,
    // the ResolutionTable, every module's tables -- and writes the
    // structured form: an object per function, each body by a
    // FunctionLowerer, then the verifier. 'finish' flattens it and verifies
    // again (questions 78 and 88), so what is left is a CFG ready to run.
    //
    // It is built one construct at a time, in the six steps record 0077
    // lists, and a construct it does not lower yet is refused by name --
    // 'not lowered yet: ...' -- never written as something else.
    class IrLowerer {
        public:
            IrLowerer();

        public:
            void set_compilation(Compilation* compilation);
            void set_target(const Target* target);
            void set_program(IrProgram* program);

            // the structured form, verified. False when something could not
            // be lowered, or the IR did not verify, with the reason in
            // get_error
            bool lower();

            // flatten, and the verifier again: what is left is a CFG ready to
            // run (questions 78 and 88)
            bool finish();

            const std::string& get_error();

        public:
            // for the FunctionLowerer
            Compilation* get_compilation();
            IrTypeTranslator* get_translator();

            // the object a function's candidate became, 0 when none
            IrObjectId function_of(u32 module, u32 candidate);

            // the IR type of a Haard type, and 0 when it has none (void)
            IrTypeId type_of(u32 module, u32 type);

            void fail(const std::string& message);

        private:
            bool declare_functions();
            bool lower_bodies();
            bool verify(const std::string& when);
            bool is_generic(u32 module, u32 declaration);

        private:
            Compilation* compilation;
            const Target* target;
            IrProgram* program;
            IrTypeTranslator translator;

            // (module, candidate) to the function's object, and each body
            // still to lower as (module, node)
            std::map<std::pair<u32, u32>, u32> functions;
            std::vector<std::pair<u32, u32>> bodies;
            std::map<std::string, u32> names;

            std::string error;
    };
}

#endif
