#ifndef HAARD_FUNCTION_LOWERER_H
#define HAARD_FUNCTION_LOWERER_H

#include <haard/ir/ir_builder.h>
#include <haard/lowering/ir_lowerer.h>

namespace haard {
    // One function's body, into the structured form (record 0077, stage 4).
    //
    // Every local is a slot (question 82), every slot at the top of the body
    // (question 13), a parameter copied into one on entry. A value never
    // leaves the region it was made in, so what has to -- the answer of an
    // 'and' -- goes through a slot too.
    //
    //   while c: B         loop $l { if not c { break $l }; B }
    //   for h; c; i: B     h; loop $l { if not c { break $l };
    //                                   block $b { B }; i }
    //
    // A 'continue' in a 'for' leaves $b, so the increment still runs.
    class FunctionLowerer {
        public:
            FunctionLowerer(IrLowerer* owner, IrProgram* program);

        public:
            // false at the first thing it cannot lower, which it has
            // reported to its owner
            bool lower(u32 module, u32 node, IrObjectId function);

        private:
            void lower_block(u32 node);
            void lower_statement(u32 node);
            void lower_if(u32 node);
            void lower_branches(u32 condition, u32 block, u32 next, u32 node);
            void lower_while(u32 node);
            void lower_for(u32 node);
            void lower_binding(u32 node);
            void lower_declaring_assignment(u32 node);
            void lower_return(u32 node);

            // an expression's value, in the IR type of its Haard type
            IrValueId value_of(u32 node);
            IrValueId literal(u32 node);
            IrValueId call(u32 node);
            IrValueId conversion(u32 node, u32 operand, u32 to);
            IrValueId arithmetic(u32 node);
            IrValueId comparison(u32 node);
            IrValueId logical(u32 node);
            IrValueId assignment(u32 node);
            IrValueId increment(u32 node);

            // where a name that can be written lives: its slot
            IrValueId place_of(u32 node);

            // the opcode of an arithmetic or a bitwise operator on this type
            IrOpcode opcode_of(AstNodeKind kind, u32 type);
            IrValueId apply(AstNodeKind kind, u32 type, IrValueId left,
                            IrValueId right, IrPosition at);
            IrValueId power(u32 type, IrValueId base, IrValueId exponent,
                            IrPosition at);

            u32 type_at(u32 node);
            IrTypeId ir_type(u32 type);
            bool is_signed_type(u32 type);
            bool is_float_type(u32 type);
            bool is_whole_type(u32 type);
            IrPosition at(u32 node);
            AstNodeKind kind_of(u32 node);
            u32 child_of(u32 node, u32 which);
            std::string text_of(u32 node);

            void not_yet(const std::string& what);

        private:
            IrLowerer* owner;
            IrProgram* program;
            Compilation* compilation;
            IrBuilder build;

            u32 module_index;
            Module* module;
            IrTypeId result;

            // a local's or a parameter's candidate, in this module, to its slot
            std::map<u32, IrValueId> slots;

            // the loops around what is being lowered, innermost last: the
            // region a 'break' leaves and the one a 'continue' does
            struct Loop {
                IrInstId broken;
                IrInstId continued;
                bool continue_breaks;
            };

            std::vector<Loop> loops;
            bool failed;
    };
}

#endif
