#ifndef HAARD_OVERLOAD_RESOLVER_H
#define HAARD_OVERLOAD_RESOLVER_H

#include <haard/type_table/type_builder.h>

namespace haard {
    // One argument as it was written, which is more than its type: record 0018
    // makes a literal take the type its context asks for, and the context here
    // is the parameter of whichever candidate is being tried. So a literal
    // arrives untyped and is asked to fit each parameter in turn
    struct Argument {
        u32 type;
        u32 node;
        bool literal;
    };

    typedef enum OverloadStatus {
        OVERLOAD_FOUND,
        OVERLOAD_NONE,
        OVERLOAD_AMBIGUOUS,
    } OverloadStatus;

    struct Overload {
        OverloadStatus status;
        u32 module;
        u32 candidate;

        // the return type, already translated into the calling module's table
        u32 result;

        // and the parameters of the candidate that won, translated too, in
        // source order and without the return. Record 0019 needs them: a
        // literal argument is carried in untyped and only ever asked to FIT a
        // parameter, never typed, so the signature that won is the one place
        // its type exists
        std::vector<u32> parameters;
    };

    // The second half of agenda 2.7: choosing among the candidates a name
    // gathered, by the signature written at the call.
    //
    // Record 0018 is what makes this short. There is no conversion to rank, so
    // an argument matches a parameter when the two types are **equal**, when
    // the argument is a literal that can be that parameter, or when it is a
    // reference or pointer to something derived from it. Nothing else matches,
    // and the only score is how many steps up the inheritance chain each
    // argument took.
    //
    // Two candidates with the same score are an ambiguous **call**, reported
    // at the call, which is what record 0012 said and not the scope's problem.
    //
    // Default parameter values make arity a **range** (record 0012), so a
    // candidate answers to any count between its required parameters and all
    // of them, and two candidates may both answer to one count.
    class OverloadResolver {
        public:
            OverloadResolver();

        public:
            void set_compilation(Compilation* compilation);

            // 'caller' is the module the call is written in, and every type in
            // 'arguments' and in the answer belongs to its table
            Overload choose(u32 caller,
                            const std::vector<Candidacy>& candidates,
                            const std::vector<Argument>& arguments);

        private:
            // how well one candidate answers, or -1 for not at all. The score
            // is the total distance climbed, so 0 is an exact match
            int score(u32 caller, const Candidacy& candidacy,
                      const std::vector<Argument>& arguments);

            int match(u32 caller, const Argument& argument, u32 parameter);

            // how many steps from one class up to another, or -1 when it is
            // not above it at all. Single inheritance makes this a walk
            int distance(u32 caller, u32 from, u32 to);

            // Hadley, 2026-09-02: a method a derived class writes with the
            // same parameters as one of its bases **overrides** it and does
            // not join it as an overload. Every method is virtual, so there is
            // one of them at a call and it is the most derived one.
            //
            // Applied here and not where the candidates were gathered, because
            // this is the only place that has the signatures to compare with,
            // and because both ways of reaching a method -- a bare name inside
            // the class and a '.' from outside -- end up here
            bool overrides(u32 caller, const Candidacy& derived,
                           const Candidacy& base);

            // the class a candidate is a method of, as a type, and
            // INVALID_TYPE when it is not a method. The way up is the scope
            // the function opened, whose parent is the class body
            u32 holder_of(const Candidacy& who);

            // a candidate's parameters, without the return, translated into
            // the caller's table so two of them can be compared
            std::vector<u32> parameters_of(u32 caller, const Candidacy& who);

            // the parameters a candidate cannot do without: those the source
            // wrote no default for
            u32 required_of(u32 module, u32 candidate);

            bool fits(u32 caller, const Argument& argument, u32 builtin);

        private:
            Compilation* compilation;
            TypeBuilder builder;
    };
}

#endif
