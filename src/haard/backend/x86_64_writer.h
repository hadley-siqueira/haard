#ifndef HAARD_BACKEND_X86_64_WRITER_H
#define HAARD_BACKEND_X86_64_WRITER_H

#include <haard/ir/ir_layout.h>
#include <haard/ir/ir_program.h>

#include <map>
#include <ostream>
#include <sstream>
#include <string>

namespace haard {
    // A flattened program as text for hasm (record 0084, step 3).
    //
    // No register allocation yet, which the record decides: every value lives
    // in eight bytes of the frame, zero extended from its width, exactly as
    // the interpreter holds it. An instruction loads what it needs, works in
    // 64 bits, cuts the result to its width and stores it. So the rules of
    // each width are the interpreter's, and x86-64's partial registers never
    // come up. It is slow on purpose: the first goal is a correct result.
    //
    // The calling convention is record 0084's: rdi, rsi, rdx, rcx, r8, r9,
    // the rest on the stack, the result in rax, and only rsp and rbp kept.
    //
    // It refuses what it does not write yet, by name, rather than guess.
    class X8664Writer {
        public:
            X8664Writer();

        public:
            void set_program(IrProgram* program);
            void set_target(const Target* target);

            // the whole program. False, with nothing written and the reason
            // in get_error, when something in it is not written yet
            bool write(std::ostream& out);
            const std::string& get_error();

        private:
            bool write_function(IrObjectId object);
            bool write_instruction(IrFunction& function, IrInstId id);
            bool write_binary(const IrInst& inst);
            void write_division(const IrInst& inst, u32 width);
            void write_shift(const IrInst& inst, u32 width);
            void write_comparison(const IrInst& inst, u32 width);
            bool write_call(IrFunction& function, IrInstId id);
            bool lay_out_frame(IrFunction& function);
            void write_order(const char* label,
                             const std::vector<IrObjectId>& functions,
                             bool backwards);

            // the value's place in the frame
            std::string place(u32 value);
            void load(const char* reg, u32 value);
            void store_result(const IrInst& inst);

            // rax or rcx, as it is held, read as a signed number of this
            // width; and rax cut down to this width
            void widen(const char* wide, u32 width);
            void mask(u32 width);

            u32 width_of(IrTypeId type);
            bool is_float(IrTypeId type);
            std::string block_label(IrBlockId block);
            std::string fresh_label();
            std::string quoted(const std::string& name);
            void line(const std::string& text);
            bool refuse(const std::string& what);

        private:
            IrProgram* program;
            const Target* target;
            LayoutTable layouts;
            std::ostringstream text;
            std::string error;

            // the function being written
            u32 current;
            u32 frame_size;
            std::map<u32, u32> slot_offsets;
            u32 fresh;
    };
}

#endif
