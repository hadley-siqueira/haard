#ifndef HAARD_IR_FLATTENER_H
#define HAARD_IR_FLATTENER_H

#include <haard/ir/ir_program.h>

namespace haard {
    // From the structured form to the CFG (question 78): one way, and the
    // last thing loop passes see.
    //
    //   block $r { B }       B, then a block after it; 'break $r' goes there
    //   if %c { T } else { E }
    //                        cond_br to T's block and E's (or to the block
    //                        after, with no 'else'); each falls into the
    //                        block after
    //   loop $r { B }        a header block that B starts in, and a block
    //                        after; 'continue $r' goes back to the header,
    //                        'break $r' after it, and falling off the end of
    //                        B goes round again (Rust's 'loop')
    //
    // The header carries the loop's bound (question 84), and every block
    // knows the innermost loop it is in, which is what counts a loop's turns.
    // A value keeps its number, so the CFG reads against the structured
    // form; the jumps flatten writes take the position of their region
    // (question 86). A block nothing jumps to and nothing was put in is
    // dropped: it is the place after an 'if' whose two sides both left.
    //
    // The function is rebuilt and not edited, so nothing here removes or
    // replaces an instruction: question 91's compaction and alias table are
    // still waiting for the first pass that does.
    class IrFlattener {
        public:
            IrFlattener();

        public:
            void set_program(IrProgram* program);

            // a CFG is left as it is
            void flatten(IrObjectId function);

        private:
            struct Frame {
                u32 region;
                IrBlockId after;

                // a loop's header, 0 for any other region
                IrBlockId top;
            };

            void flatten_block(IrBlockId block);
            void copy(IrInstId id);
            void jump(IrBlockId target, IrPosition at);
            void branch(u32 condition, IrBlockId chosen, IrBlockId otherwise,
                        IrPosition at);
            IrInstId append(const IrInst& inst, IrPosition at);

            // a block that is not in the printed order until it is placed,
            // which is when the code it holds starts being written
            IrBlockId make_block();
            void place(IrBlockId block);
            void close();
            bool ended();
            bool falls();
            const Frame& frame_of(u32 region);

        private:
            IrProgram* program;
            IrFunction in;
            IrFunction out;
            IrBlockId current;

            // the innermost loop's header, 0 outside every loop
            IrBlockId loop;
            std::vector<Frame> frames;
            std::vector<u32> jumps_to;
    };
}

#endif
