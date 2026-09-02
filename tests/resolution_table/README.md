# ResolutionTable tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Every directory in `cases/` is a project. The golden is **the table the type
phase filled**, walked in node order: one line per node the phase wrote
something about, with its type and the declaration it names.

```
    49    take          <none>              -> take  f64 -> f64
    51    2.5           f64
    52    (             f64
```

That is record 0019 in three lines. `take` names a declaration and has no type
of its own; the literal has the type of the parameter that won; the call has
the return.

## The node index is part of the line on purpose

It is what makes a golden notice a recording landing in the wrong **place**.
A call's chosen overload belongs on the name that was called and not on the
`(` above it, and the only way a golden can tell those apart is by saying which
node it went to.

## What each case pins

| case | what it proves |
|---|---|
| `every_expression_has_a_type` | every literal, name, operator, comparison, logical operator and parenthesis is written down — the funnel in `type_of` means a kind cannot be forgotten |
| `a_name_says_which_declaration_it_meant` | a local, a parameter, a field through `.`, a field through `->`, a method and a global each name their own declaration |
| `a_call_says_which_overload_it_chose` | three `take`s, and the two calls pick different ones. The golden prints the **signature** of what was chosen, because three lines reading `-> take` would say nothing — and saying which one is the whole reason the answer is kept |
| `an_override_is_not_an_overload` | record 0020: `Square.describe` overrides `Shape.describe`, `Shape.name(tag)` is still reachable, and `Square.name(tag, extra)` is a real overload told apart by arity. The golden names the **class** each method belongs to, because that is the only thing that distinguishes an override from what it overrides |
| `a_base_member_is_reachable_by_a_bare_name` | inside a method of `Square`, `sides` is Shape's field and `describe()` is Shape's method, with no dot written |
| `a_base_field_shadows_a_global` | a name found in a base ends the walk outward exactly as one found in the class itself does |
| `two_methods_of_one_class_still_tie` | an override is about **two** classes; one class declaring the same method twice has overridden nothing and is still ambiguous |
| `a_tie_across_the_boundary_is_still_ambiguous` | a base method and a derived one with **different** parameters are two overloads, and a tie between them is reported |
| `an_override_may_change_the_return_type` | the return is out of what makes an override, so `Square.describe` **is** the override and the call resolves to it — and giving back something else is then a mistake about the return, reported by the `OverrideChecker`. Both halves in one golden |
| `what_could_not_be_worked_out_is_nothing` | a name that resolves to nothing and an operator whose operands disagree write nothing, and the diagnostics are the same ones as before |

## The literal in a call is the interesting one

`take(2.5)` and `take(70000)` are the case that could not be got right by
recording as the typer walks. Record 0018 carries a literal into a call
**untyped**, so that each candidate may ask it to be its own parameter, and
`OverloadResolver` checks the digits itself without going back through the
typer. So the literal's node is never typed on the way in.

Its type only exists once a candidate has won, which is why `Overload` carries
the winner's parameters back and the call writes them in afterwards. Take that
out and the two literals lose their types while everything else still passes.

## What has a declaration but no type

The name on the right of a `.`, and the name a call calls. Both name a
declaration, and neither is an expression with a type of its own: the type is
on the `.` and on the `(` above them. A reader that wants the field's type
reads it off the candidate.
