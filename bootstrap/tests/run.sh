#!/usr/bin/env bash
# The bootstrap, judged by the C++ compiler's own goldens.
#
#   tests/run.sh      build it and run every case of each suite it has a twin of
#
# There is nothing to update here and no expected/ of its own: the answer to
# every case is already written down, by the C++ phase each tool is the twin
# of. A case passes when the two outputs are the same byte for byte.
#
#   tests/scanner   build/dump_tokens      the token stream, diagnostics first
#   tests/parser    build/parse_and_print  the diagnostics, the source printed
#                                          back, the tree, and the round trip
#
# Each suite runs from its own directory and names a case 'cases/<name>.hd',
# because that is how its goldens were written: a diagnostic quotes the path
# it was given.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")/.."

bootstrap=$(pwd)
build=${TMPDIR:-/tmp}/haard-bootstrap-tests

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

indented() {
    sed -n '1,20p' | sed "s/^/        ${dim}/;s/\$/${reset}/"
}

# the manifest and the roots table are two spellings of one build, so they
# must give the same programs before either is worth testing
if ! out=$(make -s table 2>&1); then
    printf '%sFAIL%s  haard.pkg and roots.tbl build different programs\n' \
        "$red" "$reset"
    printf '%s\n' "$out" | indented
    exit 1
fi

if ! out=$(make -s 2>&1); then
    printf '%sFAIL%s  the bootstrap does not build\n' "$red" "$reset"
    printf '%s\n' "$out" | indented
    exit 1
fi

mkdir -p "$build"

passed=0
failed=0

# suite <name> <tool> <takes a reprint path>
suite() {
    local name=$1
    local tool=$bootstrap/build/$2
    local reprint=$3
    local oracle=$bootstrap/../tests/$name
    local cases=("$oracle"/cases/*.hd)
    local total=${#cases[@]}
    local done=0

    echo
    echo "$name, against tests/$name/expected"

    cd "$oracle" || exit 1

    for case in "${cases[@]}"; do
        local case_name
        local got
        local status
        local difference

        case_name=$(basename "$case" .hd)
        done=$((done + 1))

        if [ "$reprint" = yes ]; then
            got=$(timeout 5 "$tool" "cases/$case_name.hd" \
                  "$build/$case_name.reprint.hd" 2>&1)
        else
            got=$(timeout 5 "$tool" "cases/$case_name.hd" 2>&1)
        fi

        status=$?

        if [ $status -eq 124 ]; then
            printf '[%3d/%d] %sFAIL%s  %s (timed out after 5s)\n' \
                "$done" "$total" "$red" "$reset" "$case_name"
            failed=$((failed + 1))
            continue
        fi

        if ! difference=$(diff -u "expected/$case_name.txt" \
                              <(printf '%s\n' "$got")); then
            printf '[%3d/%d] %sFAIL%s  %s\n' "$done" "$total" "$red" "$reset" \
                "$case_name"
            printf '%s\n' "$difference" | indented
            failed=$((failed + 1))
            continue
        fi

        printf '[%3d/%d] %sPASS%s  %s\n' "$done" "$total" "$green" "$reset" \
            "$case_name"
        passed=$((passed + 1))
    done

    cd "$bootstrap" || exit 1
}

suite scanner dump_tokens no
suite parser parse_and_print yes

echo

total=$((passed + failed))

if [ $failed -gt 0 ]; then
    printf '%s%d of %d passed, %d failed%s\n' "$red" "$passed" "$total" \
        "$failed" "$reset"
    exit 1
fi

printf '%s%d of %d passed%s\n' "$green" "$passed" "$total" "$reset"
