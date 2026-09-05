#ifndef HAARD_EMITTER_H
#define HAARD_EMITTER_H

#include <haard/type_table/expression_typer.h>
#include <ostream>
#include <set>
#include <sstream>

namespace haard {
    // Haard as C++, for a C++ compiler to turn into a binary.
    //
    // The point is not readable output. It is that a language with no back end
    // gets one, and that **every architecture with a C++ compiler becomes a
    // target** — haard -> C++ -> cc -> binary. A real back end comes later and
    // will want an intermediate representation; this does not, and the reason
    // it does not is record 0019: the ResolutionTable already holds a type and
    // a declaration for every node, so the tree is annotated and the emitter
    // never has to work anything out. It is the PrettyPrinter's discipline with
    // a table in hand.
    //
    // ## The whole program in one file, in four sections
    //
    // Haard lets a name be used before it is declared and C++ does not, so the
    // output is ordered rather than the input: every type forward declared,
    // then the types, then every function prototyped, then the globals, then
    // the bodies. Inside
    // the second section the order still matters -- a base class and a field
    // held by value have to be complete -- so it is a walk over those two
    // edges, and a cycle in it is reported rather than emitted.
    //
    // ## Every name is mangled, and that is not for tidiness
    //
    // A name becomes 'h<module>_<candidate>_<source name>'. Two reasons, both
    // of them things that go wrong silently otherwise:
    //
    //   Record 0010 puts **two versions of one library in one program**, so
    //   two modules declare 'zip.compress' and they are different code. Source
    //   names collide.
    //
    //   A **method** is named differently, and it has to be: 'm_<name>_<its
    //   parameters>'. Record 0020 says a derived class writing a base's name
    //   and parameters **overrides** it, and in C++ two virtual functions with
    //   different names are two functions -- the derived one would compile,
    //   run, and never be called through a base pointer. Naming a method by
    //   what makes it that method is that rule, written once.
    //
    //   C++ resolves overloads by its own rules, which are not record 0018's:
    //   no numeric conversion here, a ranking by inheritance depth, a literal's
    //   distance from its default type. A call this compiler resolved to one
    //   candidate could resolve to another there. Emitting the candidate's own
    //   name means C++ never chooses anything.
    //
    // A C++ keyword can never be produced either, which is the third thing it
    // buys and the reason locals and parameters are mangled too.
    //
    // ## Sugar
    //
    // What is desugared is desugared **here**, locally, because every piece of
    // it so far is local: 'elif' is 'else if', 'and' and 'not' are '&&' and
    // '!' whichever spelling was written, and a '.' on a pointer is '->'.
    //
    // 'for x in a..b' is **not** here, and the reason is worth writing down:
    // a range has no type (agenda 2.10 leaves it to a decision), so the loop
    // variable has none, so **no program using one gets past the type phase**
    // and no case could reach an emitter for it. Writing one would be code no
    // test could break. It goes in when the range does.
    //
    // When something arrives that needs a name the source never wrote, or a
    // statement where an expression was, it wants a tree-to-tree pass before
    // this one rather than a bigger switch inside it -- the PrettyPrinter is
    // already an oracle for a tree, and an intermediate representation would
    // need a dumper and a suite of its own.
    class Emitter {
        public:
            Emitter();

        public:
            void set_compilation(Compilation* compilation);

            // false when nothing could be emitted, with the reason in
            // get_error. Everything it cannot do says so rather than writing
            // C++ that does something else
            bool emit(std::ostream& out);

            const std::string& get_error();

        private:
            // the sections, in the order they are written
            void emit_forward_declarations();
            void emit_types();
            void emit_prototypes();
            void emit_globals();
            void emit_bodies();

            // a type and everything it needs complete before it: its base and
            // every field it holds by value. The set is what makes it a walk
            // and not a loop that repeats
            void emit_type(u32 module, u32 declaration);

            void emit_field(u32 module, u32 node);
            void emit_method_declaration(u32 module, u32 node);
            void emit_function_body(u32 module, u32 node, u32 holder);
            // 'defaults' writes each parameter's default value, which C++
            // takes once and at the declaration. Record 0012 makes arity a
            // range, so leaving them out turns a call the language allows into
            // one C++ has no function for
            void emit_signature(u32 module, u32 node, const std::string& name,
                                bool defaults);
            void emit_parameters(u32 module, u32 node, bool defaults);
            void emit_argument_names(u32 module, u32 node);

            // the C++ constructor and destructor a class's 'init' and
            // 'destroy' become, declared inside the struct and defined outside
            // it like every other method
            void emit_structors(u32 module, u32 declaration,
                                const std::string& holder, bool bodies);
            void emit_main();

            void emit_statement(u32 module, u32 node);
            void emit_block(u32 module, u32 node);
            void emit_if(u32 module, u32 node);
            void emit_while(u32 module, u32 node);
            void emit_for(u32 module, u32 node);
            void emit_for_part(u32 module, u32 part);
            void emit_binding(u32 module, u32 node);

            // 'a = 1' where nothing declared 'a': a declaration in C++, and
            // its left side is not a use so it is not in the ResolutionTable
            void emit_declaring_assignment(u32 module, u32 node);

            void emit_expression(u32 module, u32 node);
            void emit_binary(u32 module, u32 node, const std::string& oper);
            void emit_unary(u32 module, u32 node, const std::string& oper);
            void emit_postfix(u32 module, u32 node, const std::string& oper);
            void emit_call(u32 module, u32 node);
            void emit_call_arguments(u32 module, u32 arguments);
            u32 second_child_of(u32 module, u32 node);
            void emit_member(u32 module, u32 node, bool arrow);
            void emit_identifier(u32 module, u32 node);

            // a declaration's name in the output. The module and the candidate
            // are what make it unique, and the source name is there so that a
            // reader of the C++ can find their way back
            std::string name_of(u32 module, u32 candidate);

            // the same, for the declaration a node names, and the empty string
            // when the node names none
            std::string name_at(u32 module, u32 node);

            // a method's parameters as a string every module spells the same
            // way, which is what makes an override carry the base's name
            std::string mangle_parameters(u32 module, u32 candidate);
            std::string mangle_type(u32 module, u32 type);

            // 'type name', which is one string and not two because C++ writes
            // an array's length after the name. Every declaration goes through
            // it: a field, a parameter, a local
            std::string declare(u32 module, u32 type, const std::string& name);
            std::string type_name(u32 module, u32 type);

            // the class a candidate is a method of, and 0 when it is not one.
            // A bare name inside a method reaches a field and a method of the
            // class and of its bases (record 0020), and both have to be
            // written 'this->' in C++
            u32 holder_of(u32 module, u32 candidate);

            // record 0026: a value comes into being running its class's 'init'
            // with no arguments, and a class whose every 'init' needs one
            // cannot. Refused here by name, since there is no syntax yet for
            // writing a base's arguments
            void require_default_construction(u32 module, u32 type,
                                              const std::string& where);

            bool is_pointer(u32 module, u32 node);
            u32 type_at(u32 module, u32 node);

            // the type a declaration was given, which lives on its candidate and
            // not in the ResolutionTable
            u32 declared_type(u32 module, u32 node);

            // whether this declaration is a generic the source wrote, as
            // opposed to a clone record 0002 made of one. The originals are
            // not emitted: they name type parameters nothing bound
            bool is_generic(u32 module, u32 declaration);

            void fail(const std::string& message);

            AstNodeKind kind_of(u32 module, u32 node);
            u32 child_of(u32 module, u32 node, u32 which);
            std::string text_of(u32 module, u32 node);
            void line(const std::string& text);

        private:
            Compilation* compilation;
            std::ostringstream out;
            std::string error;
            u32 indentation;

            // the declarations already written, as (module, declaration), so
            // the walk over what a type needs complete visits each once
            std::set<std::pair<u32, u32>> emitted;
            std::set<std::pair<u32, u32>> emitting;
    };
}

#endif
