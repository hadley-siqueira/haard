#ifndef HAARD_USE_RESOLVER_H
#define HAARD_USE_RESOLVER_H

#include <haard/name_resolver/name_resolver.h>
#include <map>
#include <set>

namespace haard {
    // Walks a module and resolves every name it *uses*, reporting the ones
    // nothing declares. The first diagnostic this family produces: until now
    // the resolver answered questions a test asked, and nothing asked.
    //
    // It knows which scope it is in without any new data, by reading
    // Scope::owner backwards: the collector opened one scope per function,
    // block, closure, for-each and type body, and stamped the node that opened
    // it. So the walk switches scope when it steps into a node that owns one,
    // and the two passes cannot drift apart -- there is one description of the
    // scope shape and this is not a second copy of it.
    //
    // What counts as a use is decided by structure and not by a list of
    // exceptions, which is what keeps it from rotting as the grammar grows:
    //
    //   an AST_BINDING_NAME is a declaration, wherever it appears. Every
    //   declaration wraps its name in one -- a function, a class, a field, a
    //   parameter, a binding, a closure parameter -- so one rule skips them
    //   all, and a new declaration written the same way is covered for free
    //
    //   AST_GENERIC_PARAMETERS declares, and a for-each's loop variables
    //   declare, so both are skipped in the same spirit
    //
    //   the right side of an AST_DOT or an AST_ARROW is NOT resolved here. It is a field of
    //   whatever the left side turns out to be, so it needs a type, and
    //   sending it through the scope chain would find a global of its name in
    //   silence. That is the trap this pass exists to not fall into
    //
    //   an AST_SCOPE is one of the two qualified forms and goes to the
    //   resolver's own entry points for them
    //
    //   a label and a goto name a namespace this table does not model, so
    //   neither is resolved and neither is reported
    class UseResolver {
        public:
            UseResolver();

        public:
            void set_compilation(Compilation* compilation);

            // resolves every use in this module, logging one error per name
            // nothing declares
            void resolve(u32 module);

        private:
            void walk(u32 node, u32 scope);

            // 'alias::name' and '::name', which the parser tells apart by how
            // many children the node has
            void walk_scope(u32 node, u32 scope);

            void use(u32 node, u32 scope);
            void report(u32 node, const std::string& message);

            std::string text_of(u32 node);
            AstNodeKind kind_of(u32 node);

        private:
            Compilation* compilation;
            NameResolver resolver;

            AstQuery query;

            Module* module;
            u32 index;

            // the node that opened a scope, back to the scope it opened
            std::map<u32, u32> scope_of;

            // the identifiers a for-each binds. They are the one declaration
            // not wrapped in an AST_BINDING_NAME, so they cannot be skipped by
            // shape and are collected as the walk reaches each loop
            std::set<u32> declarations;
    };
}

#endif
