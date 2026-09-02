# SymbolTable tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every file in `cases/` is scanned, parsed and collected, and the golden is the
**scope tree**: a scope with its owner and its parent, the names in it in
declaration order, and under each name its candidates in declaration order.

```
scope 1  owner the module  parent 0
  Node
    class at node 2
  get
    function at node 39
scope 2  owner Node  parent 1
  value
    field at node 10
  get
    function at node 18
    function at node 26
```

The shape of the dump is the shape of the data, which is the point: a name with
two candidates under it *is* record 0012's decision that a name maps to a list
and not to one.

## What the goldens are pinning

**That overloading exists.** `two_functions_of_one_name` has two `def foo` and
the golden shows one name with two candidates. Nothing in the compiler could
show this before — it is the first time record 0012 is observable rather than
written down.

**That a scope is not a filter over one table.** `a_class_opens_a_scope` has a
`get` in the class and a `get` at module level, and they are two symbols in two
scopes, not one symbol with three candidates. The class's two `get`s *are* one
symbol with two candidates, in scope 2.

**That the names of a scope come out in source order.** The first version of the
collector made a pass per kind — every function, then every class — and
`a_class_opens_a_scope` came out with `get` before `Node` though the source
wrote `Node` first. Reading that golden is what found it. It matters because
record 0009 resolves a name to the first declaration that answers, so a name
list in an order the source never wrote is a name list that can answer wrongly.

**That `Scope.owner` and `Scope.parent` are real.** Both are checked by
`every_kind_in_the_module_scope`, where four type declarations open four scopes,
each parented to the module scope and each owned by its declaration. Without the
owner the dump cannot say which body a field is in.

**That a tuple binding declares every name it writes.**
`a_tuple_binding_declares_two_names` has `let (a, b) = pair`, and both names
point at the **same** binding node — an identifier has no way back up to the
type it was written with, because an `AstNode` carries children and siblings and
no parent.

**That `pass` declares nothing.** `an_empty_body_declares_nothing` still opens
the class's scope, and the scope is empty.

**That a broken module is never collected.**
`a_broken_module_is_never_collected` prints diagnostics and nothing else, which
is the phase gate every reader of an `Ast` relies on.

## The case that is deliberately not an error

`a_function_and_a_variable_share_a_name` declares `def value` and `let value`.
The golden shows one name with two candidates of different kinds, **and no
diagnostic** — record 0013 left open whether that is an error and where it is
reported, and this phase decides nothing. The table can now hold it, which is
what makes it reportable later; the case is here so that whichever way 0013's
question is answered, the answer changes a golden that already exists.

## Inside a function

`a_function_holds_its_params_and_locals`, `a_block_opens_a_scope`,
`a_closure_declares_its_parameters` and `generic_parameters_are_names` cover
the body walk, and three shapes are worth stating because they were choices:

**A function opens two scopes, not one.** The first holds its generic
parameters and its parameters; its block opens a second below it. So a local of
a parameter's name **shadows** the parameter instead of standing beside it as a
second candidate, which is what the rule of 2026-09-02 does everywhere else.

**Every `AST_BLOCK` opens a scope, and the walk knows no statement kinds.** An
`if`, a `while` and a `for` all carry their block as a child, so the collector
recurses over children and opens a scope wherever it finds a block. Nothing has
to be added when a statement kind is. `a_block_opens_a_scope` has an `if` and a
`while` under one function and gets three block scopes, the inner one parented
to the outer.

**A closure is the only thing inside an expression that declares anything.** It
is reached by the same recursion, opens a scope for its parameters, and its
braced body is an ordinary `AST_BLOCK` that opens one more.

**A generic parameter is the one declaration whose name is not wrapped in an
`AST_BINDING_NAME`** — the identifier *is* the node, which is both the name and
what the symbol points at.

## The sabotages

Each is the number of the eight cases that fail:

| sabotage | fails |
|---|---|
| a candidate is prepended instead of appended (overloads in reverse) | 3 |
| `declare` always makes a new symbol, so overloads never group | 3 |
| a type body reuses the enclosing scope instead of opening one | 3 |
| a scope is opened with no owner | 3 |
| every member is a field, so a method is not seen as a function | 1 |
| a tuple binding declares only its first name | 1 |
| a block opens no scope, so locals land in the function scope | 9 |
| a function opens no scope, so parameters land beside the function | 9 |
| the recursion passes the outer scope, so nesting flattens | 3 |
| parameters are not collected | 4 |
| generic parameters are not collected | 1 |
| a closure declares nothing | 1 |
| loop variables are not collected | 1 |
| only the left of the `in` is bound, its siblings ignored | 1 |
| the whole `in` is bound instead of its left side | 1 |

## The for-each, and the shape it forced

Hadley decided on 2026-09-02 that **the collector reads the loop variables off
the head**, rather than the parser marking them as bindings.

The head is not one node. It is a list of comma separated expressions whose last
one is the `in`, so `for key, value in pairs` writes `key` as a **sibling** of
the `AST_IN` and `value` as its **left child**. Both are bound;
`a_for_each_binds_its_variables` is the case.

Three things it settles by showing them:

- The loop opens a scope of its own with the block below it, the same pair a
  function makes with its parameters, so `for i in 0..10:` with a `let i` inside
  is a shadow and not two candidates.
- Both names point at the **loop node**, not at their identifiers — from a name
  there is no way back to the sequence it comes out of, which is the same reason
  a tuple binding's names point at their binding.
- **A C shaped `for j = 0; j < 10; ...` binds nothing.** Its head is an
  assignment, not an `in`, so `j` has to be declared before the loop. The golden
  records that, which is the point of having the case hold all three loops.

Everything a body can declare is now collected: parameters, generic parameters,
`let` and `const` at any depth, a closure's parameters, and a for-each's loop
variables.
