# NameResolver tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project, like the compilation suite's, plus a
`queries` file of one question per line:

```
cases/<name>/
    table.tbl        the roots table
    entry            one line: the file to start from
    queries          <module name> <scope> <name>, one per line
    <the sources>    real Haard, compiled by the real compilation
```

`<scope>` is `module` for the module's own scope, or the name of a class,
struct, enum or union for the scope of its body. The golden is the candidate
set: every declaration the name may mean, **in the order the resolver gathered
them**, each named by its module, its kind and its node.

```
app.main module -> println
  app.main function println at node 2
  std.io function println at node 2
```

The node is printed because two candidates of one name in one module are
otherwise the same line, and their order is the whole point of record 0009.

## This suite is the first thing that joins the pieces

Before it, three things had unit tests and no caller: `StringTable::find`, which
is the probe across a module boundary; `Module::get_dependencies`, which is the
import order record 0009 reads; and the `SymbolCollector` itself. Every case
here needs all three, and the compilation now runs the collector.

## What the goldens are pinning

**That the import does not merge.** `a_name_comes_from_an_import` resolves
`println` from `app.main` and finds it in `std.io`'s own table — record 0013's
claim, and the first time it runs. The name crosses as a hash and its bytes,
which is what record 0014's hash is for.

**That the string table answers before the scopes do.** The same case asks for
`nobody`, which no module interned, and the walk is skipped entirely.

**That the importer's own declaration comes first.**
`the_importer_comes_first` has a `println` in both, and both are candidates with
the importer's first — record 0008 as amended by record 0012, which made the
importer's declaration stop hiding an import's and start merely preceding it.

**That "the first import wins" means first in the source.**
`two_imports_bring_one_name` writes `import lib.second` above `import lib.first`,
and `lib.second` comes back first. Sorting the names would have reversed it.

**That a star's expansion is sorted.** `a_star_import_brings_the_directory` gets
`std.io` before `std.list`, which record 0006 requires and record 0009 turned
into something that decides which declaration a program means.

**That an import is not transitive.** In `an_import_is_not_transitive`,
`app.main` imports `lib.middle` which imports `lib.bottom`, and `deep` is
invisible from `app.main` and visible from `lib.middle`. Record 0008.

**That the scope steps contribute to one set.**
`overloads_gather_across_scopes` asks a class scope for `get` and gets three
candidates: the class's two overloads and the module's one. Record 0012.

**That a non function shadows and a function does not.**
`a_field_shadows_a_method_does_not` is Hadley's rule of 2026-09-02, which record
0012 had left open for the class boundary: the field `value` hides the module's
`def value`, while the method `get` joins the module's `def get` in one set.

**That the walk runs the whole chain.** `a_local_scope_reaches_an_import` asks
from the innermost block of a generic function and reaches, in one query each:
the block's own local, the enclosing block, the parameter, the generic
parameter, and `println` in `std.io`. Nothing else in the suite exercises more
than two of those steps.

**That a block is a scope.** `a_local_shadows_a_param` has a `let arg` inside an
`if` and an `@arg` parameter outside it: from the inner block the local answers
alone, from the outer one the parameter does. With no block scope both come
back for the inner question.

**That a for-each binds both of its names and neither escapes.**
`a_loop_variable_resolves` writes `for key, value in pairs` next to a module
level `def key`: from inside the loop the variable answers alone, and from the
module scope `value` is not there at all. A loop variable also shadows a
parameter of its name, the same way a local does.

## The two qualified forms

A query writes the name the way a programmer writes it, and the scope field is
ignored for both forms.

**`alias::name`** reaches only the imports written under that alias, and they
are the same imports the bare form already reaches — an alias **adds** a way to
name something and never withholds it (records 0008 and 0009).
`an_alias_names_its_import` shows all of it in one golden: `p1::shared` and
`p2::shared` name one module each, the bare `shared` still brings all three in
import order, `third::shared` finds nothing because that import has no `as`, and
an alias nobody wrote finds nothing.

**A star's alias names the pool and not a module** (record 0006), so
`a_star_alias_names_the_pool` has `io::shared` come back as ambiguous as the
bare form. Note what the bare form does there: `std.tcp` appears **twice**,
because `import std.* as io` and `import std.tcp as tcp` bring it by two routes.
Record 0009 calls that one declaration arriving twice, with first-wins choosing
between equals, and the golden records it rather than hiding it.

**`::name` needed no logic of its own.** Record 0009 makes the leading `::` skip
the local scope and the enclosing class, which is the ordinary walk started from
the module scope. The grammar had this the moment the walk took a starting
scope. `the_module_scope_form` is record 0009's own example: from a method,
`foo` gives the class's and the module's, and `::foo` gives only the module's.

### One reading that is an interpretation, not a decision

`the_module_scope_form` also asks `::println`, where `println` comes from an
import, and gets it. Record 0009 says `::name` names *"what the module declares
at its top level"*, and record 0008 makes an import bind exactly there — so `::`
is read here as restricting where the walk **starts**, not where candidates may
come from. The other reading, that `::` means the module's **own** declarations
and excludes imports, is defensible. The query is in the golden so that
answering the question flips a file instead of going unnoticed.

## No name is printed per candidate

The golden lines say the module, the kind and the node, and never a name. That
is the data model and not an omission: record 0013 puts the name on the
**Symbol**, and a candidate is only ever reached through one. Two loop variables
of one `for ... in` point at the same node and are told apart by the symbols
above them — as are the two names of `let (a, b) = pair`. An earlier version of
this dumper tried to derive a name from the candidate's node and printed two
blank ones, which is what the shape had been saying all along.

## The sabotages

Each is the number of the seven cases that fail:

| sabotage | fails |
|---|---|
| the collector is never run by the compilation | 7 |
| a dependency is asked with the importer's interned index | 5 |
| the walk always stops at the first scope that answers | 3 |
| dependencies are visited in reverse order | 2 |
| the walk never stops, so a field does not shadow | 1 |
| a dependency's own imports are searched too | 1 |
| the alias is not kept, so every import is aliasless | 2 |
| a qualified form ignores the alias and searches every import | 2 |
| a star gives its alias only to the first file of its expansion | 1 |
| `::name` starts where the name was written | 1 |
| an import with no `as` gets a qualified form from its last segment | 1 |

The fourth one is the point of the whole design: an interned index is
meaningful only inside the module that made it, and using the importer's on a
dependency's table finds whatever happens to sit at that index.

## What this does not do

**It gathers and it never decides.** Two candidates that a call would find
equally good are an ambiguous *call*, which record 0012 puts at the call; two
imports bringing one name are record 0009's warning at the use. Both come back
in this list and neither is reported here.

Choosing among candidates by signature is the second half of agenda 2.7 and
waits on the type table (2.6) and on the logic of implicit conversion (1.13).

**One sabotage this suite does not catch:** a function opening no scope, so its
parameters land in the module scope. Every query here still answers the same,
because the walk reaches the module scope anyway. The symbol table suite catches
it — 9 of its 12 cases — which is the split between the two suites: that one
pins the shape, this one pins what the shape answers.
