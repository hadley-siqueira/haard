#ifndef HAARD_IR_PRINTER_H
#define HAARD_IR_PRINTER_H

#include <haard/ir/ir_program.h>

#include <ostream>

namespace haard {
    // The IR as text, for people and goldens (record 0077: a dump for
    // reading, which nothing parses back).
    //
    //   %4    a value: the function's parameters first, then what each
    //         instruction gives back, numbered as they were made
    //   @f    an object: a function, a datum
    //   $7    a region of the structured form, by its instruction
    //   ^2    a block of a CFG
    //
    // The type is written on the instruction (question 80), and an
    // instruction prints the way record 0077's stage 2 table names it.
    class IrPrinter {
        public:
            IrPrinter();

        public:
            void set_program(IrProgram* program);

            // each instruction followed by where in the source it came from
            void set_show_positions(bool show);

            // the verifier's culprit, marked '>>' in its function
            void set_mark(IrObjectId function, IrInstId inst);

            void print(std::ostream& out);

        private:
            bool print_types(std::ostream& out);
            void print_data(std::ostream& out, IrObjectId id);
            void print_function(std::ostream& out, IrObjectId id);
            void print_block(std::ostream& out, IrFunction& function,
                             IrBlockId block, u32 depth);
            void print_inst(std::ostream& out, IrFunction& function,
                            IrInstId id, u32 depth);

            // '  ; at module:offset' when positions are shown
            std::string where(IrFunction& function, IrInstId id);

            std::string value(IrValueId id);
            std::string value(u32 index);
            std::string type(IrTypeId id);
            std::string block_name(IrFunction& function, IrBlockId id);
            std::string constant(IrFunction& function, IrTypeId type,
                                 u32 index);
            std::string arguments(IrFunction& function, IrInstId id);
            std::string signature(u32 index);
            std::string slot_name(IrTypeId type, u32 slot);

        private:
            IrProgram* program;
            bool show_positions;
            IrObjectId marked_function;
            IrInstId marked;
    };
}

#endif
