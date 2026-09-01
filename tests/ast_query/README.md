# AstQuery tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every file in `cases/*.hd` is scanned and parsed by `query.cpp`, which then asks
`AstQuery` what the module declares and writes the answers to
`expected/<case>.txt`: the imports with their star and their alias, the
functions, the classes, the structs, the enums, the unions, and the `let` and
`const` statements.

## What the goldens are actually pinning

**That a declaration is a child of the root and never deeper.** The tempting
implementation is a loop over `Ast::get_nodes()`, since the nodes are a flat
vector and their indices are already in source order. It gives the right answer
for imports, because an import cannot nest — and the wrong one for everything
else. `a_method_is_not_a_function.hd` is the case that says so: a method is an
`AST_FUNCTION` too and a local is an `AST_LET_DECLARATION` too, so the flat scan
reports two functions and three variables where the module declares one of each.
Walking the root's children is also the cheaper of the two, since it touches a
few dozen declarations instead of every literal and every operator in the file.

**That `let` and `const` come back in source order.** They are collected in one
pass over the children and not in two passes concatenated, or every `let` would
sort ahead of every `const`. `let_and_const_keep_source_order.hd` is the case.

**That a name hangs two levels down.** An `AST_BINDING_NAME` carries no token of
its own; it wraps the `AST_IDENTIFIER` that does. Reading the wrapper gives
token 0, which is the first token of the file — so the bug prints `import` as
the name of every function. It was caught by reading the golden, not by a crash.

**That the star is not part of the name.** `import std.io.*` is named `std.io`,
with the star answered separately: it says where to look, not what to look for.

## The phase gate

Every method on `AstQuery` assumes the tree parsed clean, and nothing in it
checks. That is not an oversight — `Driver::compile` stops on
`!module.get_logger()->has_errors()`, so a module that failed to parse never
reaches a query. It matters because a statement that fails is dropped by
`parse_module` without its nodes being taken back: `import a.` leaves an
`AST_IMPORT` in the vector that no parent points at. The walk cannot see it, but
a flat scan would, and would go looking for a file the source never named.

`a_broken_module_is_never_queried.hd` writes that contract down: the golden is
the diagnostic, and no answers at all.

## One case that pins nothing

`nothing_is_declared.hd` is a module with only a comment in it. No sabotage of
`AstQuery` makes it fail, because `Ast::get_node` gives back the sentinel for
index 0 and the walk ends before it starts. It is here as a crash guard on the
empty tree, not as a recorded behaviour — worth knowing before wondering why it
never moves.

## Reading the golden diff

**`./run.sh -u` accepts whatever `AstQuery` produces, bugs included.** Step one
is running it; the step that gives the test its value is reading
`git diff expected/` afterwards. Two of the goldens here were wrong the first
time they were written, and only reading them caught it.

The per-case `timeout` is a test of its own: a walk that forgets to follow the
sibling link never ends, and shows up as a failure instead of hanging the suite.
