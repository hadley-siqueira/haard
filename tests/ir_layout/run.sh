#!/usr/bin/env bash
# IR layout tests. Dependencies: g++, bash, diff and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per case
#   ./run.sh -u       rewrite the expected/ files with the current output
#
# Every directory in cases/ is a project: an optional roots table, an entry
# file, the sources and an optional 'target' file (x86_64-linux when there is
# none). layout.cpp translates the program's types into the IR's and lays
# them out for that target, and the result is compared against
# expected/<case>.txt.
#
# Then g++ is asked. On an x86-64 machine, for an x86_64-linux case, the C++
# the emitter writes is built with a probe that prints what sizeof, alignof
# and offsetof say about every aggregate, and that has to be what the IR's
# layout says, byte for byte. Hadley chose the Itanium ABI's rules on
# 2026-10-20 so that this check could cover classes with a base too. A case
# whose golden is an error has nothing to probe.
#
# Read the diff before committing it: -u accepts whatever it produces, bugs
# included.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-ir-layout-tests
update=0

if [ "${1:-}" = "-u" ]; then
    update=1
fi

if [ -t 1 ] && [ -z "${NO_COLOR:-}" ]; then
    green=$'\033[32m'
    red=$'\033[31m'
    dim=$'\033[2m'
    reset=$'\033[0m'
else
    green=''
    red=''
    dim=''
    reset=''
fi

sources=(
    "$root/src/haard/token/token.cpp"
    "$root/src/haard/token_stream/token_stream.cpp"
    "$root/src/haard/ast/ast.cpp"
    "$root/src/haard/ast/ast_builder.cpp"
    "$root/src/haard/ast/ast_node.cpp"
    "$root/src/haard/ast_query/ast_query.cpp"
    "$root/src/haard/module_finder/module_finder.cpp"
    "$root/src/haard/manifest/manifest.cpp"
    "$root/src/haard/manifest/package_loader.cpp"
    "$root/src/haard/sugar/sugar_lowerer.cpp"
    "$root/src/haard/sugar/for_each_lowerer.cpp"
    "$root/src/haard/sugar/switch_lowerer.cpp"
    "$root/src/haard/compilation/compilation.cpp"
    "$root/src/haard/type_table/type_collector.cpp"
    "$root/src/haard/type_table/coercion.cpp"
    "$root/src/haard/type_table/expression_typer.cpp"
    "$root/src/haard/name_resolver/overload_resolver.cpp"
    "$root/src/haard/type_table/type_builder.cpp"
    "$root/src/haard/type_table/instantiator.cpp"
    "$root/src/haard/name_resolver/name_resolver.cpp"
    "$root/src/haard/name_resolver/use_resolver.cpp"
    "$root/src/haard/statement_checker/statement_checker.cpp"
    "$root/src/haard/override_checker/override_checker.cpp"
    "$root/src/haard/symbol_table/symbol_collector.cpp"
    "$root/src/haard/symbol_table/implicit_collector.cpp"
    "$root/src/haard/source_file/source_file.cpp"
    "$root/src/haard/log/log.cpp"
    "$root/src/haard/scanner/scanner.cpp"
    "$root/src/haard/parser/parser.cpp"
    "$root/src/haard/module/module.cpp"
    "$root/src/haard/type_table/type_table.cpp"
    "$root/src/haard/resolution_table/resolution_table.cpp"
    "$root/src/haard/string_table/string_table.cpp"
    "$root/src/haard/symbol_table/symbol_table.cpp"
    "$root/src/haard/emitter/emitter.cpp"
    "$root/src/haard/ir/target.cpp"
    "$root/src/haard/ir/ir_type_table.cpp"
    "$root/src/haard/ir/ir_layout.cpp"
    "$root/src/haard/ir/ir_type_translator.cpp"
)

mkdir -p "$build" expected

. "$root/tests/objects.sh"

# the compiler's sources, compiled once for the whole test run and shared with
# every other suite. See tests/objects.sh
if ! haard_objects "$root" "${sources[@]}"; then
    exit 1
fi

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/layout" \
        layout.cpp "${HAARD_OBJECTS[@]}" 2>&1); then
    printf '%sERROR%s failed to build the case runner\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

# the probe's sizes are the machine's, so it is asked only on the machine the
# x86_64-linux target describes
host=$(uname -m)

cases=(cases/*/)
total=${#cases[@]}
passed=0
failed=0
done=0

echo

for directory in "${cases[@]}"; do
    name=$(basename "$directory")
    golden=expected/$name.txt
    done=$((done + 1))
    target=x86_64-linux

    if [ -f "cases/$name/target" ]; then
        target=$(head -n 1 "cases/$name/target")
    fi

    got=$(timeout 5 "$build/layout" "cases/$name" 2>&1)

    if [ $? -eq 124 ]; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s (timed out after 5s)\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
        failed=$((failed + 1))
        continue
    fi

    if [ $update -eq 1 ]; then
        printf '%s\n' "$got" > "$golden"
        printf '%s[%2d/%d]%s %sWROTE%s %s\n' \
            "$dim" "$done" "$total" "$reset" "$green" "$reset" "$golden"
        continue
    fi

    if [ ! -f "$golden" ]; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s (%s is missing; run ./run.sh -u)\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name" "$golden"
        failed=$((failed + 1))
        continue
    fi

    if ! diff -u "$golden" <(printf '%s\n' "$got") > "$build/$name.diff"; then
        printf '%s[%2d/%d]%s %sFAIL%s  %s\n' \
            "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
        sed -n '1,20p' "$build/$name.diff" | sed "s/^/           ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    checked=""

    if [ "$target" = x86_64-linux ] && [ "$host" = x86_64 ] \
            && ! grep -q '^error:' "$golden"; then
        "$build/layout" "cases/$name" --probe > "$build/$name.cpp" 2>&1
        "$build/layout" "cases/$name" --facts > "$build/$name.facts" 2>&1

        if ! err=$(g++ -std=c++17 -w -o "$build/$name.probe" \
                "$build/$name.cpp" 2>&1); then
            printf '%s[%2d/%d]%s %sFAIL%s  %s (the probe did not build)\n' \
                "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
            printf '%s\n' "$err" | sed -n '1,20p' | sed "s/^/           /"
            failed=$((failed + 1))
            continue
        fi

        if ! diff -u "$build/$name.facts" <(timeout 5 "$build/$name.probe") \
                > "$build/$name.probe.diff"; then
            printf '%s[%2d/%d]%s %sFAIL%s  %s (g++ lays it out otherwise)\n' \
                "$dim" "$done" "$total" "$reset" "$red" "$reset" "$name"
            sed -n '1,30p' "$build/$name.probe.diff" \
                | sed "s/^/           ${dim}/;s/\$/${reset}/"
            failed=$((failed + 1))
            continue
        fi

        checked=" ${dim}(g++ agrees)${reset}"
    fi

    printf '%s[%2d/%d]%s %sPASS%s  %s%s\n' \
        "$dim" "$done" "$total" "$reset" "$green" "$reset" "$name" "$checked"
    passed=$((passed + 1))
done

if [ $update -eq 1 ]; then
    echo
    echo "expected/ rewritten. Review the diff with 'git diff' before committing."
    exit 0
fi

echo
if [ $failed -gt 0 ]; then
    printf '%s%d of %d passed, %d failed%s\n' "$red" "$passed" "$total" "$failed" "$reset"
    exit 1
fi

printf '%s%d of %d passed%s\n' "$green" "$passed" "$total" "$reset"
