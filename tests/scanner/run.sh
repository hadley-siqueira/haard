#!/usr/bin/env bash
# Scanner tests. Dependencies: g++, bash, diff and timeout. Nothing external.
#
#   ./run.sh          run everything, one line per case
#   ./run.sh -u       rewrite the expected/ files with the current output
#
# Progress is printed as it happens: every compilation step and every case gets
# its own line, prefixed with a [done/total percent] counter. On a terminal the
# step currently running is shown on a transient line that the result overwrites,
# so a slow or hanging step is visible while it is still running.
#
# Colors turn themselves off when stdout is not a terminal, or with NO_COLOR=1
# in the environment.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-scanner-tests
update=0

if [ "${1:-}" = "-u" ]; then
    update=1
fi

if [ -t 1 ]; then
    tty=1
else
    tty=''
fi

if [ -n "$tty" ] && [ -z "${NO_COLOR:-}" ]; then
    green=$'\033[32m'
    red=$'\033[31m'
    yellow=$'\033[33m'
    dim=$'\033[2m'
    reset=$'\033[0m'
else
    green=''
    red=''
    yellow=''
    dim=''
    reset=''
fi

# prefix <done> <total>: the [ 3/21  14%] counter
prefix() {
    printf '[%*d/%d %3d%%]' "${#2}" "$1" "$2" "$(($1 * 100 / $2))"
}

# live <done> <total> <label> <text>: transient line for a step in flight.
# Only on a terminal, and only worth it for steps that take a moment: it is
# overwritten by the result line, so a pipe or a log file never sees it.
live() {
    [ -n "$tty" ] || return 0
    printf '\r%s%s %-5s %s%s\033[K' "$dim" "$(prefix "$1" "$2")" "$3" "$4" "$reset"
}

# result <done> <total> <color> <label> <text>: the line that stays
result() {
    [ -n "$tty" ] && printf '\r\033[K'
    printf '%s%s%s %s%-5s%s %s\n' "$dim" "$(prefix "$1" "$2")" "$reset" "$3" "$4" "$reset" "$5"
}

# indented block (diff, invariant message) printed under a failing case
detail() {
    [ -n "$1" ] && printf '%s\n' "$1" | sed "s/^/                 ${dim}/;s/\$/${reset}/"
}

sources=(
    "$root/src/haard/token/token.cpp"
    "$root/src/haard/token_stream/token_stream.cpp"
    "$root/src/haard/ast/ast.cpp"
    "$root/src/haard/ast/ast_builder.cpp"
    "$root/src/haard/ast/ast_node.cpp"
    "$root/src/haard/source_file/source_file.cpp"
    "$root/src/haard/log/log.cpp"
    "$root/src/haard/scanner/scanner.cpp"
    "$root/src/haard/context/context.cpp"
)

drivers=(dump_tokens check_invariants)

mkdir -p "$build/obj" expected

# The scanner sources are shared by both drivers, so compile them to objects
# once and link twice, instead of handing the same seven files to g++ twice.
# Each object is also one visible step of progress.
build_total=$((${#sources[@]} + ${#drivers[@]}))
build_done=0
objects=()

for src in "${sources[@]}"; do
    obj="$build/obj/$(basename "$src" .cpp).o"
    live "$build_done" "$build_total" "CC" "$(basename "$src")"

    if ! err=$(g++ -std=c++20 -I"$root/src" -c -o "$obj" "$src" 2>&1); then
        build_done=$((build_done + 1))
        result "$build_done" "$build_total" "$red" "ERROR" "failed to compile $src"
        detail "$err"
        exit 1
    fi

    objects+=("$obj")
    build_done=$((build_done + 1))
    result "$build_done" "$build_total" "$green" "CC" "$(basename "$src")"
done

for driver in "${drivers[@]}"; do
    live "$build_done" "$build_total" "LD" "$driver"

    if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/$driver" "$driver.cpp" "${objects[@]}" 2>&1); then
        build_done=$((build_done + 1))
        result "$build_done" "$build_total" "$red" "ERROR" "failed to build $driver"
        detail "$err"
        exit 1
    fi

    build_done=$((build_done + 1))
    result "$build_done" "$build_total" "$green" "LD" "$driver"
done

# cases that document known bugs; see known_failures.txt
known=$(grep -v '^#' known_failures.txt 2>/dev/null | grep -v '^$')

is_known() {
    printf '%s\n' $known | grep -qx "$1"
}

cases=(cases/*.hd)
total=${#cases[@]}

if [ "$total" -eq 0 ]; then
    echo "no cases in cases/"
    exit 1
fi

# a case listed in known_failures.txt fails as XFAIL, without breaking the suite
report_failure() {
    if is_known "$1"; then
        result "$done" "$total" "$yellow" "XFAIL" "$1 (known bug, see known_failures.txt)"
        xfailed=$((xfailed + 1))
    else
        result "$done" "$total" "$red" "FAIL" "$1 ($2)"
        detail "$3"
        failed=$((failed + 1))
    fi
}

passed=0
failed=0
xfailed=0
done=0

echo

for case in "${cases[@]}"; do
    name=$(basename "$case" .hd)
    golden=expected/$name.txt
    live "$done" "$total" "RUN" "$name"
    done=$((done + 1))

    # golden test: the output must match the committed expected/ file.
    # the timeout doubles as a regression test against infinite loops
    got=$(timeout 5 "$build/dump_tokens" "$case" 2>&1)

    if [ $? -eq 124 ]; then
        report_failure "$name" "dump_tokens timed out after 5s" ""
        continue
    fi

    if [ $update -eq 1 ]; then
        printf '%s\n' "$got" > "$golden"
        result "$done" "$total" "$green" "WROTE" "$golden"
        continue
    fi

    if [ ! -f "$golden" ]; then
        report_failure "$name" "$golden is missing; run ./run.sh -u to create it" ""
        continue
    fi

    if ! diff -u "$golden" <(printf '%s\n' "$got") > "$build/$name.diff"; then
        report_failure "$name" "output differs from $golden" \
            "$(sed -n '1,20p' "$build/$name.diff")"
        continue
    fi

    # round-trip and whitespace invariants, independent of expected/
    if ! out=$(timeout 5 "$build/check_invariants" "$case" 2>&1); then
        report_failure "$name" "invariants" "$out"
        continue
    fi

    if is_known "$name"; then
        result "$done" "$total" "$red" "XPASS" \
            "$name (passed, but is listed in known_failures.txt; drop the line)"
        failed=$((failed + 1))
        continue
    fi

    result "$done" "$total" "$green" "PASS" "$name"
    passed=$((passed + 1))
done

if [ $update -eq 1 ]; then
    echo
    echo "expected/ rewritten. Review the diff with 'git diff' before committing."
    exit 0
fi

echo
if [ $failed -gt 0 ]; then
    printf '%s%d of %d passed, %d failed%s, %d known failures (XFAIL)\n' \
        "$red" "$passed" "$total" "$failed" "$reset" "$xfailed"
    exit 1
fi

printf '%s%d of %d passed%s, %d failed, %d known failures (XFAIL)\n' \
    "$green" "$passed" "$total" "$reset" "$failed" "$xfailed"
