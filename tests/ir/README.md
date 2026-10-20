# IR tests

No frameworks. Only needs `g++`, `bash`, `diff` and `timeout`.

```
./run.sh          # run everything
./run.sh -u       # rewrite the expected/ files with the current output
```

Or, with the other suites, from the build directory: `make check`.

Stage 2 of the IR (record 0077): the storage, the `IrBuilder`, the
`IrPrinter` and the `IrVerifier`. Nothing lowers Haard to the IR yet, so every
case is a C++ function in `cases.cpp` that builds a program by hand, and the
golden is the program printed followed by what the verifier says.

## Two kinds of case

**Well formed**, ending in `verifier: ok`: every integer operation, the floats
and the conversions, the constants and the data objects, memory and
aggregates, the three calls, the barriers, nested regions with `break` and
`continue`, a CFG with block parameters and a `switch`, an `abort` with the
positions shown, and the effects table printed whole. These pin the printer's
text and the builder's shapes.

**Broken on purpose**, named `verifier_...` (question 88): one rule each. The
golden holds the message and the program with the culprit marked `>>`:

```
func @f() -> b32 {
  $1: block {
    %1 = const b32 1
  }
>>ret b32 %1
}

verifier:
    @f, ret #3: its value operand %1 is not defined before it, around it
```

## Order matters in a case

C++ leaves the order of a call's arguments unspecified, so two instructions
built inside one argument list could come out in either order -- and did, in
two cases, before they were rewritten. A case builds one instruction per
statement wherever two of them would otherwise race.

## Sabotage

Each rule of the verifier was switched off, one at a time, and the suite run;
every one failed its case, and the ones that guard operands failed several
(table below). The effects table (question 85) is checked when the compiler is
built, by a `static_assert` on its length: an entry removed is a compile
error, which was tried too. A restored file is touched afterwards, or the
shared object cache keeps the sabotaged object.

| Switched off | Fails |
|---|---|
| a CFG jump in a structured function | `verifier_a_jump_in_a_structured_function` |
| a region in a CFG | `verifier_a_region_in_a_cfg` |
| something after a terminator | `verifier_something_after_a_terminator` |
| a block that does not end in a terminator | `verifier_a_block_that_does_not_end` |
| an empty block | `verifier_an_empty_block` |
| a jump to the entry | `verifier_a_jump_to_the_entry` |
| a jump to a block of no function | `verifier_a_jump_out_of_the_function` |
| block arguments counted | `verifier_too_few_block_arguments` |
| block arguments typed | `verifier_a_block_argument_of_the_wrong_type` |
| `cond_br` to a block with parameters | `verifier_cond_br_to_a_block_with_parameters` |
| a `break` out of a region around it | `verifier_a_break_out_of_no_region_around_it` |
| a `continue` of a loop only | `verifier_a_continue_of_a_block` |
| operand types at all | five cases |
| a float operation on floats | `verifier_a_float_operation_on_whole_numbers` |
| an ordered comparison on whole numbers | `verifier_an_ordered_comparison_of_floats` |
| a widening that widens | `verifier_a_widening_that_narrows` |
| the kinds a conversion takes | `verifier_a_conversion_of_the_wrong_kind` |
| a `select`'s condition a bool | `verifier_a_select_on_a_number` |
| a constant a number or a bool | `verifier_a_constant_pointer` |
| no load of an aggregate | `verifier_a_load_of_an_aggregate` |
| no copy of a scalar | `verifier_a_copy_of_a_scalar` |
| a field that is there | `verifier_a_field_that_is_not_there` |
| an index a whole number | `verifier_an_index_by_a_float` |
| an upcast to a base | `verifier_an_upcast_to_what_is_not_a_base` |
| a call's arity | `verifier_a_call_with_too_many_arguments` |
| a call to a function | `verifier_a_call_to_data` |
| a `vcall` on a class | `verifier_a_vcall_on_a_struct` |
| a `vcall`'s slot there | `verifier_a_vcall_of_a_slot_that_is_not_there` |
| a `vcall` with its slot's signature | `verifier_a_vcall_with_another_signature` |
| a value defined before its use | `verifier_a_value_used_before_it_is_made` |
| the structured form's scope rule | the two structured cases |
| the CFG's dominance | `verifier_a_value_from_a_branch_not_taken` |
| a return with its value | `verifier_a_return_with_nothing` |
| an `abort`'s message read-only | `verifier_an_abort_with_a_writable_message` |
| a position on every instruction | `verifier_an_instruction_from_nowhere` |
| a bound on a real-time loop | `verifier_a_real_time_loop_with_no_bound` |
