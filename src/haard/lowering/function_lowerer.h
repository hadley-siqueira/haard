#ifndef HAARD_FUNCTION_LOWERER_H
#define HAARD_FUNCTION_LOWERER_H

#include <haard/ir/ir_builder.h>
#include <haard/lowering/ir_lowerer.h>

#include <set>

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
    //
    // A place -- what can be written, or have its address taken -- is an
    // address: a local's slot, a global's object, what a pointer points at,
    // an element. A 'T&' is the address of what it names (record 0035), so
    // a reference local is a slot holding a pointer, and reading one is a
    // step through it; a 'T&' parameter is given the place of its argument,
    // or a temporary's when the argument is not one.
    //
    // A fixed array is never a value either: where one is wanted it is the
    // address of its first element, C's decay, which record 0018 lists.
    //
    // Nor is a struct (question 80): it is built where it goes -- a slot, a
    // field, an allocation, the destination a caller passed -- by its
    // constructor, a copy or a call. What has to end is pushed on a cleanup
    // stack (question 83): a local on its block's scope, a temporary on its
    // statement's, and each scope's run last to first wherever it is left,
    // a 'return' leaving them all, the value given back written first. A
    // temporary made on one side of an 'and' or an 'or' only has a flag.
    class FunctionLowerer {
        public:
            FunctionLowerer(IrLowerer* owner, IrProgram* program);

        public:
            // false at the first thing it cannot lower, which it has
            // reported to its owner
            bool lower(u32 module, u32 node, IrObjectId function);

            // the body of a module's initialiser: each global given its
            // value, in source order
            bool lower_globals(u32 module, IrObjectId function);

            // what C++ writes for a struct without being asked
            bool lower_structor(const StructorJob& job);

        private:
            void begin(u32 module, IrObjectId function);
            void lower_block(u32 node);
            void lower_statement(u32 node);
            void lower_if(u32 node);
            void lower_branches(u32 condition, u32 block, u32 next, u32 node);
            void lower_while(u32 node);
            void lower_for(u32 node);
            void lower_binding(u32 node);
            void lower_declaring_assignment(u32 node);
            void lower_return(u32 node);

            // the cleanup stack: a block's scope, or a statement's, whose
            // temporaries end with it
            void open_scope(bool statement);
            void close_scope(IrPosition at);

            // every scope down to 'depth' run, none closed: what a 'return',
            // a 'break' and a 'continue' leave
            void run_cleanups(u32 depth, IrPosition at);

            // what ends 'place', a 'type', when its scope does: a local's on
            // its block, a temporary's on its statement
            void add_cleanup(IrValueId place, IrTypeId type, bool local,
                             IrPosition at);

            // the one local every 'return' gives back, built straight in the
            // destination (C++'s named return value), 0 when there is none
            u32 named_result_of(u32 body);
            void returns_in(u32 node, std::vector<u32>& found);

            // what 'given' means, written into 'place', which holds a 'type'
            void initialise(IrValueId place, u32 type, u32 given,
                            IrPosition at);

            // an expression's value, in the IR type of its Haard type
            IrValueId value_of(u32 node);
            IrValueId literal(u32 node);
            // 'destination' is where a struct given back is built, 0 for a
            // temporary
            IrValueId call(u32 node, IrValueId destination);
            IrValueId conversion(u32 node, u32 operand, u32 to);
            IrValueId arithmetic(u32 node);
            IrValueId comparison(u32 node);
            IrValueId logical(u32 node);
            IrValueId assignment(u32 node);
            IrValueId increment(u32 node);
            IrValueId string_literal(u32 node);
            IrValueId size_of(u32 node);
            IrValueId allocation(u32 node);
            IrValueId release(u32 node);

            // a braced literal where a value is wanted: a temporary array,
            // decayed
            IrValueId brace_literal(u32 node);

            // a struct: built into 'place', which holds nothing yet, from
            // what 'node' means
            void build_into(IrValueId place, IrTypeId type, u32 node);

            // a temporary of the node's struct, built and ended with its
            // statement; its address
            IrValueId materialise(u32 node);

            // record 0037: a bracket or braced literal a class takes, a
            // fixed array of its elements given to an 'init' taking a
            // pointer and a count -- or, for an 'init' taking one value,
            // given to the one that builds that value first
            void build_from_literal(IrValueId place, IrTypeId type, u32 node);

            void construct(IrValueId place, IrTypeId type, u32 init,
                           const std::vector<IrValueId>& arguments,
                           IrPosition at);
            void construct_by_default(IrValueId place, IrTypeId type,
                                      IrPosition at);
            void copy_construct(IrValueId place, IrTypeId type, IrValueId from,
                                IrPosition at);

            // a field at a time, as C++'s implicit '=' does, the base too
            void copy_fields(IrTypeId type, IrValueId to, IrValueId from,
                             IrPosition at);

            // every byte of 'place' zero, which C++'s 'new T()' does first
            // for a struct with no 'init'
            void zero(IrValueId place, IrTypeId type, IrPosition at);

            // a loop over a fixed array's elements, first to last or last
            // to first: 'begin_elements' opens it and gives the element's
            // address, 'end_elements' closes it
            IrValueId begin_elements(IrValueId place, IrTypeId array,
                                     bool backwards, IrValueId& turn,
                                     IrPosition at);

            // the same over 'length' elements, a usize known only when it
            // runs: what 'new T[n]' and 'delete[]' walk
            IrValueId begin_elements(IrValueId place, IrTypeId element,
                                     IrValueId length, u32 bound,
                                     bool backwards, IrValueId& turn,
                                     IrPosition at);

            // whether 'new T[n]' keeps n before the elements, as the
            // Itanium ABI's cookie does: when 'delete[]' has to end each
            bool keeps_count(IrTypeId element);
            void end_elements(IrValueId turn, IrPosition at);

            // a parameter's default, written where the function is declared
            // and worked out where it is called, as C++ does
            IrValueId default_of(u32 module, u32 parameter, bool reference);

            // what a call is given for each argument, by the Haard types of
            // the callee's parameters: a place for a 'T&', a copy the caller
            // made for a struct, the value for the rest
            // at most 'limit' of them, from 'first' on
            bool arguments_for(u32 module, u32 candidate, u32 first,
                               std::vector<IrValueId>& values,
                               u32 limit = 0xffffffff);

            // record 0034: an operator a struct declares, a call to it with
            // the left operand as the object
            IrValueId operator_call(u32 node, IrValueId destination);

            // the field a candidate names, reached from a pointer to a
            // 'type', up through the bases
            IrValueId field_of(IrValueId pointer, IrTypeId type, u32 module,
                               u32 candidate, IrPosition at);

            // a pointer to a 'from' seen as the 'to' it derives from
            IrValueId upcast_to(IrValueId pointer, IrTypeId from, IrTypeId to,
                                IrPosition at);

            // a class's vptr pointed at its own vtable's slot 0: what C++
            // does at the start of each constructor and each destructor, so
            // a call made from one runs this class's method
            void store_vptr(IrValueId object, IrTypeId type, IrPosition at);

            // the destructor of a 'type' called on 'place'; a class's takes
            // its object as the class at the top of its vtable's chain
            void destroy(IrValueId place, IrTypeId type, IrPosition at);

            // what C++'s implicit '=' does to one held in a field: a struct's
            // bytes, a class field by field, so its vptr stays its own
            void assign_value(IrTypeId type, IrValueId to, IrValueId from,
                              IrPosition at);

            // a method called directly, or through its slot when it fills
            // one and 'virtually' says so
            IrValueId call_method(u32 module, u32 candidate,
                                  const std::vector<IrValueId>& values,
                                  bool virtually, IrPosition at);

            // a place's value: a load, or a fixed array's decay
            IrValueId read(u32 node);

            // 'p + n' and 'p - n', n elements on, and 'q - p' the elements
            // between
            IrValueId step(u32 pointer, IrValueId base, IrValueId count,
                           u32 count_type, bool back, IrPosition at);
            IrValueId distance(u32 pointer, IrValueId left, IrValueId right,
                               IrPosition at);

            // where something that can be written lives
            IrValueId place_of(u32 node);
            IrValueId place_of_name(u32 node);
            IrValueId place_of_member(u32 node);
            IrValueId place_of_index(u32 node);
            bool is_place(u32 node);

            // what a 'T&' is given: the place, or a temporary holding the
            // value when it is not one
            IrValueId reference_to(u32 node);

            // the function a call reaches, its module and candidate, and the
            // Haard types of its parameters and result; false when it is not
            // a call to a function
            bool callee_of(u32 node, u32& module, u32& candidate);

            // the object a method is called on, as a pointer to the struct
            // that declares the method
            IrValueId receiver_of(u32 callee, u32 module, u32 candidate);
            std::vector<u32> signature_of(u32 module, u32 candidate);
            bool returns_reference(u32 node);

            // the opcode of an arithmetic or a bitwise operator on this type
            IrOpcode opcode_of(AstNodeKind kind, u32 type);
            IrValueId apply(AstNodeKind kind, u32 type, IrValueId left,
                            IrValueId right, IrPosition at);
            IrValueId power(u32 type, IrValueId base, IrValueId exponent,
                            IrPosition at);

            u32 type_at(u32 node);

            // the IR type of a value of this Haard type, a reference read
            // through: a scalar, since an aggregate is never a value
            // (question 80)
            IrTypeId ir_type(u32 type);

            // what a slot or a global holding one is: a pointer for a 'T&',
            // an array for a 'T[n]'
            IrTypeId slot_type(u32 type);

            // a pointer's pointee or a fixed array's element, in the IR
            IrTypeId element_of(u32 type);

            // the struct a Haard type is, read through a reference; 0 when
            // it is not one
            IrTypeId struct_of(u32 type);
            IrTypeId struct_of(u32 type, u32 module);

            // the base made ready, then each field, as every constructor
            // does before its 'init' runs
            void prepare(IrValueId object, IrTypeId type);
            IrTypeId usize();

            // to a whole number of another width, by the sign of 'type'
            IrValueId to_width(IrValueId value, u32 type, IrTypeId to,
                               IrPosition at);

            // the kind of this type with a reference read through
            u8 kind_of_type(u32 type);
            bool is_reference_type(u32 module, u32 type);
            bool is_signed_type(u32 type);
            bool is_float_type(u32 type);
            bool is_whole_type(u32 type);
            IrPosition at(u32 node);
            AstNodeKind kind_of(u32 node);
            u32 child_of(u32 node, u32 which);
            std::string text_of(u32 node);

            // a literal's body, its escapes read: C's, and Haard's '\$'
            std::string unescape(const std::string& body);

            void not_yet(const std::string& what);

        private:
            IrLowerer* owner;
            IrProgram* program;
            Compilation* compilation;
            IrBuilder build;

            u32 module_index;
            Module* module;
            IrTypeId result;

            // the Haard type the function gives back, which says whether a
            // 'return' gives a place
            u32 returning;

            // a method's object, as the slot holding it, and its struct; the
            // destination of a struct given back, a parameter
            IrValueId self;
            IrTypeId holder;
            IrValueId destination;
            u32 named_result;

            // the parameters given by value that are structs: their slot
            // holds the address of the caller's copy
            std::set<u32> indirect;

            struct Cleanup {
                IrValueId place;
                IrTypeId type;

                // 0 for one made on every path
                IrValueId flag;
            };

            struct Scope {
                std::vector<Cleanup> cleanups;
                bool statement;
            };

            std::vector<Scope> scopes;

            // how many 'and's and 'or's the expression is inside the right
            // side of
            u32 conditional;

            // a local's or a parameter's candidate, in this module, to its slot
            std::map<u32, IrValueId> slots;

            // the loops around what is being lowered, innermost last: the
            // region a 'break' leaves and the one a 'continue' does
            struct Loop {
                IrInstId broken;
                IrInstId continued;
                bool continue_breaks;

                // the scopes around it, which its 'break' does not leave
                u32 depth;
            };

            std::vector<Loop> loops;
            bool failed;
    };
}

#endif
