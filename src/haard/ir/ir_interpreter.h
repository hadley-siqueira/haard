#ifndef HAARD_IR_INTERPRETER_H
#define HAARD_IR_INTERPRETER_H

#include <haard/ir/ir_layout.h>
#include <haard/ir/ir_program.h>

#include <map>
#include <ostream>

namespace haard {
    typedef enum IrRunKind {
        // the function gave its value back
        IR_RUN_EXIT,

        // the program stopped itself: an 'abort', a division by zero, a
        // float converted to a whole number it does not fit
        IR_RUN_ABORT,

        // the interpreter stopped it, because it did something Haard says it
        // may not or the IR cannot mean: a bug of the program or of the
        // compiler, which is the reason this is an oracle
        IR_RUN_STOP,
    } IrRunKind;

    struct IrRunResult {
        u8 kind;
        u64 value;
        std::string message;

        // where it stopped or aborted
        IrObjectId function;
        IrInstId inst;
        IrPosition at;
    };

    // Runs the CFG form (question 87; the structured form runs after
    // flatten, question 78). The test oracle: a program run here must give
    // the exit status the C++ path gives.
    //
    // Memory is by object, as Miri and CompCert keep it: every object of the
    // program, every slot of every frame and every allocation is a block of
    // its own, and a pointer is the block and an offset, packed as the block
    // in the high 32 bits -- a made-up base address that 'ptrtoint' and
    // pointer arithmetic still work on. So it stops, saying where, on:
    //
    //   - an access outside a block, through null, or to a slot of a frame
    //     that has returned
    //   - a write to a read-only object
    //   - a call through something that is not a function
    //   - a loop going round more often than its bound (question 84)
    //   - an access to an allocation after it was freed, a free of one
    //     freed already, and a free of what no allocation gave
    //
    // Bytes are in the target's order, by the target's layout. New memory is
    // filled with a fixed pattern when asked (question 10), so a program that
    // reads what nobody wrote gives an answer it cannot rely on. The natives
    // of 'std.low_io' are its devices: output goes to the stream it is given.
    // '__alloc' and '__free' are the heap's, which 'new' and 'delete' call.
    //
    // The program's initialisers run first, in their order, and then the
    // function asked for.
    class IrInterpreter {
        public:
            IrInterpreter();

        public:
            void set_program(IrProgram* program);
            void set_target(const Target* target);
            void set_output(std::ostream* output);
            void set_garbage(bool garbage);

            IrRunResult run(IrObjectId function,
                            const std::vector<u64>& arguments);

            // a program's 'main': given nothing when it takes nothing, and
            // C's pair when it takes two -- the count, and the arguments as
            // text ending in a null pointer, as C hands them over
            IrRunResult run_main(IrObjectId main,
                                 const std::vector<std::string>& arguments);

        private:
            struct Memory {
                std::vector<u8> bytes;
                bool live;
                bool read_only;

                // made by '__alloc', so '__free' may end it
                bool heap;

                // the object it holds, 0 for a slot or a device
                IrObjectId object;
                std::string what;
            };

            struct Frame {
                IrObjectId function;
                std::vector<u64> values;
                std::vector<u32> slots;

                // how many times each loop has gone round since it was
                // entered, by header
                std::map<u32, u32> turns;
            };

            // every object in its block, false when the layout failed
            bool prepare();
            IrRunResult finish(IrObjectId function,
                               const std::vector<u64>& arguments);

            u64 call(IrObjectId function, const std::vector<u64>& arguments);
            u64 native(IrObjectId function, const std::vector<u64>& arguments);
            u64 execute(Frame& frame, IrFunction& function, IrInstId id,
                        IrBlockId& next, bool& left);
            void enter(Frame& frame, IrFunction& function, IrBlockId from,
                       IrBlockId to);

            u64 arithmetic(IrOpcode opcode, IrTypeId type, u64 left,
                           u64 right);
            u64 compare(IrOpcode opcode, IrTypeId type, u64 left, u64 right);
            u64 convert(IrOpcode opcode, IrTypeId from, IrTypeId to,
                        u64 value);

            u32 allocate(u32 size, const std::string& what, bool garbage);
            u8* reach(u64 address, u32 size, bool write);
            u64 load(u64 address, IrTypeId type);
            void store(u64 address, IrTypeId type, u64 value);
            u64 address_of(IrObjectId object);

            u32 bits(IrTypeId type);
            u64 mask(u64 value, u32 width);
            i64 widen(u64 value, u32 width);

            void stop(const std::string& message);
            void abort(const std::string& message);

        private:
            IrProgram* program;
            const Target* target;
            std::ostream* output;
            bool garbage;

            LayoutTable layouts;
            std::vector<Memory> memory;
            std::vector<u32> blocks_of;
            u64 pattern;
            u32 depth;

            // the three handles of 'std.low_io'
            u64 stdout_handle;
            u64 stderr_handle;
            u64 stdin_handle;

            // where execution is, for a stop's report
            IrObjectId at_function;
            IrInstId at_inst;

            bool halted;
            IrRunResult result;
    };
}

#endif
