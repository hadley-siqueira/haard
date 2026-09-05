#!/usr/bin/env bash
# Whole-program tests. Dependencies: hdc, make, g++, bash, diff and timeout.
#
#   ./run.sh          run everything, one line per case
#   ./run.sh -u       rewrite the expected/ files with the current output
#
# Every directory in cases/ is a **project a person could have written**: a
# roots table, several roots, and a Makefile. This suite is the only one that
# runs the flow the way a user does --
#
#     hdc --roots table entry.hd --emit-cpp  ->  g++  ->  ./app
#
# -- through the case's own Makefile, and through the real 'hdc' binary rather
# than a driver built for the occasion. Which means the Makefile is not
# documentation that can rot: if it stops describing how a Haard program is
# built, this suite stops passing.
#
# The verdict is three things, in the golden: what the program **printed**,
# the **files it left behind** by name and size, and its **exit status**. The
# programs check their own work and print what held, so a golden of a few
# lines pins a great deal -- and an image that changed size shows up even if
# nothing printed differently.
#
# Each case is copied to a scratch directory first, so a program that writes
# files writes them there and the repository stays clean.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
build=${TMPDIR:-/tmp}/haard-program-tests
update=0

# the real compiler, and not one this suite builds: the point is the binary a
# person runs. Override it when the build tree is elsewhere
hdc=${HDC:-$(cd "$root" && pwd)/build/hdc}

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

if [ ! -x "$hdc" ]; then
    printf '%sERROR%s no hdc at %s -- build it first, or pass HDC=\n' \
        "$red" "$reset" "$hdc"
    exit 1
fi

rm -rf "$build"
mkdir -p "$build" expected

cases=(cases/*/)
total=${#cases[@]}
passed=0
failed=0
done=0

echo

for directory in "${cases[@]}"; do
    name=$(basename "$directory")
    golden=expected/$name.txt
    work=$build/$name
    done=$((done + 1))

    cp -r "$directory" "$work"

    # the case's own Makefile, so there is one description of the process and
    # this suite is what keeps it true
    if ! errors=$(timeout 60 make -s -C "$work" HDC="$hdc" app 2>&1); then
        got="did not build:
$errors"
    else
        printed=$(cd "$work" && timeout 30 ./app 2>&1)
        code=$?

        got="$printed
--- files:"

        # what the program left behind, by name and size. A directory, the
        # sources and the two things the Makefile builds are not artefacts
        for file in $(cd "$work" && ls -A); do
            case $file in
            main.cpp|app|Makefile|roots.tbl|entry) continue ;;
            esac

            if [ -d "$work/$file" ]; then
                continue
            fi

            got="$got
$file, $(wc -c < "$work/$file") bytes"
        done

        got="$got
--- exit: $code"
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
        sed -n '1,25p' "$build/$name.diff" | sed "s/^/           ${dim}/;s/\$/${reset}/"
        failed=$((failed + 1))
        continue
    fi

    printf '%s[%2d/%d]%s %sPASS%s  %s\n' \
        "$dim" "$done" "$total" "$reset" "$green" "$reset" "$name"
    passed=$((passed + 1))
done

echo

if [ $update -eq 1 ]; then
    echo "expected/ rewritten. Review the diff with 'git diff' before committing."
    exit 0
fi

if [ $failed -gt 0 ]; then
    printf '%d of %d passed, %s%d failed%s\n' \
        "$passed" "$total" "$red" "$failed" "$reset"
    exit 1
fi

printf '%d of %d passed\n' "$passed" "$total"
