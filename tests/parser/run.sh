#!/usr/bin/env bash
# Parser tests. Dependencies: g++, bash, diff and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per case
#   ./run.sh -u       rewrite the expected/ files with the current output
#
# Every file in cases/*.hd is scanned, parsed, printed back as source and
# dumped as a tree. The output — diagnostics, source, tree, round trip verdict —
# is compared against expected/<case>.txt. Read the diff before committing it:
# -u accepts whatever the parser produces, bugs included.
#
# The printed source is written to the build directory and parsed again, and the
# two trees are compared; that verdict is the last line of every golden.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-parser-tests
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
    "$root/src/haard/source_file/source_file.cpp"
    "$root/src/haard/log/log.cpp"
    "$root/src/haard/scanner/scanner.cpp"
    "$root/src/haard/parser/parser.cpp"
    "$root/src/haard/pretty_printer/pretty_printer.cpp"
    "$root/src/haard/module/module.cpp"
    "$root/src/haard/type_table/type_table.cpp"
    "$root/src/haard/string_table/string_table.cpp"
    "$root/src/haard/symbol_table/symbol_table.cpp"
)

mkdir -p "$build" expected

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/parse_and_print" \
        parse_and_print.cpp "${sources[@]}" 2>&1); then
    printf '%sERROR%s failed to build parse_and_print\n' "$red" "$reset"
    printf '%s\n' "$err"
    exit 1
fi

cases=(cases/*.hd)
total=${#cases[@]}
passed=0
failed=0
done=0

echo

for case in "${cases[@]}"; do
    name=$(basename "$case" .hd)
    golden=expected/$name.txt
    done=$((done + 1))

    # the timeout is a test of its own: a parser that stops making progress
    # shows up as a failure instead of hanging the suite
    got=$(timeout 5 "$build/parse_and_print" "$case" "$build/$name.reprint.hd" 2>&1)

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

    printf '%s[%2d/%d]%s %sPASS%s  %s\n' \
        "$dim" "$done" "$total" "$reset" "$green" "$reset" "$name"
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
