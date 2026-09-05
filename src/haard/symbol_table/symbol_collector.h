#ifndef HAARD_SYMBOL_COLLECTOR_H
#define HAARD_SYMBOL_COLLECTOR_H

#include <haard/ast_query/ast_query.h>
#include <haard/module/module.h>
#include <map>

namespace haard {
    // Fills a module's symbol table from its tree. The fourth of the family:
    // Ast holds the nodes, AstBuilder writes them, AstQuery reads them back,
    // and this one turns what they say into names.
    //
    // It decides nothing. A name that is declared twice is two candidates and
    // not a diagnostic, a 'def foo' next to a 'let foo' is two candidates of
    // different kinds, and nothing here looks at a type. Record 0012 put both
    // halves of resolution after this one -- gather the candidates by name,
    // then choose among them by signature -- and neither half is this phase's.
    //
    // What it does not walk: an expression, except to find a closure inside
    // one. Which names an expression *uses* is the resolver's question and not
    // this one; the only thing an expression can declare is a closure's
    // parameters.
    //
    // Like every reader of an Ast it assumes the tree parsed clean, which the
    // phase gate guarantees.
    class SymbolCollector {
        public:
            SymbolCollector();

        public:
            void set_module(Module* module);

            void collect();

            // One declaration, into a scope that already exists, under a name
            // that is given rather than read off the tree. Record 0002's
            // instantiation appends a cloned class to the tree after this
            // phase has run, and the clone needs exactly the names the first
            // walk would have given it -- under a name no source can write, so
            // that nothing resolves to it by accident.
            //
            // The implicit walk runs over the clone alone, for the same reason
            // the first one runs over the module: a body that assigns to a
            // bare name declares it, and the clone has its own body
            void collect_declaration(u32 scope, u32 declaration,
                                     const std::string& name);

        private:
            // the switch the two entries share: which of the five shapes this
            // declaration is, and where its names go
            void collect_one(u32 scope, u32 node, const std::string& name);

            // a class, a struct, an enum or a union: the declaration goes in
            // the scope it was written in, and its members go in a scope of
            // its own whose owner is the declaration
            void collect_type(u32 scope, u32 declaration, SymbolKind kind,
                              const std::string& name);

            // a function: its generic parameters and its parameters go in a
            // scope of its own, and its block opens one more below that. Two
            // scopes and not one, so a local may shadow a parameter of its
            // name instead of joining it as a second candidate
            void collect_function(u32 scope, u32 declaration,
                                  const std::string& name);

            // Everything inside a body, without knowing what a statement is.
            // An 'if', a 'while' and a 'for' all carry their block as a child,
            // so opening a scope at every AST_BLOCK covers all of them and
            // whatever statement is added next. A closure is the one thing in
            // an expression that declares anything, and it is reached the same
            // way
            void collect_statement(u32 scope, u32 node);

            void collect_closure(u32 scope, u32 closure);

            // Hadley, 2026-09-02: a 'for ... in' binds its loop variables, and
            // the collector reads them off the head rather than the parser
            // marking them. They go in a scope of the loop's own, with the
            // block below it, so a local of a loop variable's name shadows it
            // the way a local shadows a parameter
            void collect_loop_variables(u32 scope, u32 for_each);

            // a 'let' or a 'const' declares one name per name in its
            // binding, and every one of them points at the binding: an
            // identifier has no way back up to the type it was written with
            void collect_binding(u32 scope, u32 statement);

            // 'let' is not required: an assignment to a bare name that this
            // module has nowhere in view declares it, the way 'let n = 1'
            // would. It is a second walk and it cannot be part of the first:
            // 'n = 1' in the first function of a file may be naming a global
            // the file declares at its end, and a use is allowed to come
            // before its declaration.
            //
            // Only a plain '=' declares. 'n += 1' reads n before it writes it,
            // so a name it cannot find is a mistake and not a declaration, and
            // so is anything on the left that is not one identifier: 'a.b',
            // 'p->x' and 'a[i]' all name something that has to exist already.
            //
            // Only what THIS module has in view counts -- the scope chain up
            // to and including the module scope, and not the imports. A local
            // standing in front of an imported name is what 'let' does too, so
            // this stays a per-module question and the phase stays inside the
            // walk that fills one module
            void collect_implicit(u32 node, u32 scope);
            void declare_target(u32 scope, u32 target);

            bool in_view(u32 scope, u32 name);

            // the identifiers of '<T, U>'. The one declaration whose name is
            // not wrapped in a binding name, so it is its own node
            void collect_generic_parameters(u32 scope, u32 declaration);

            u32 declare(u32 scope, const std::string& name, SymbolKind kind,
                        u32 node);

        private:
            Module* module;
            AstQuery query;

            // the node that opened a scope, back to the scope it opened. The
            // same reading of Scope::owner that every later phase does, and
            // for the same reason: one description of the scope shape
            std::map<u32, u32> scope_of;
    };
}

#endif
