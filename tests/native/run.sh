#!/usr/bin/env bash
# The native back end (record 0084, step 3): hdc, hasm and the runtime, with no C.
#
#   ./run.sh
#
# A program's output and exit code are what is judged, against the two paths
# that exist already, which are its oracle:
#
#   cases/ and widths.sh   each one built and run three ways, C++, the IR's
#                          interpreter and native, all of it live: the three agree
#   ../emitter/cases       against what its golden says the C++ path did
#   ../programs/cases      the same
#
# A program the back end does not write yet is counted and not failed, and the end
# lists why, which is what is left of step 3. Anything else that differs is a failure.
#
# Exits 0 if everything passed, 1 otherwise.
set -u

cd "$(dirname "$0")"

root=../..
here=$(pwd)
build=${TMPDIR:-/tmp}/haard-native-tests
hdc=$here/$root/build/hdc
hasm=$here/$root/toolchain/hasm/build/hasm
runtime=$here/$root/toolchain/runtime/x86_64_linux.s

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
    "$root/src/haard/ir/ir_class_table.cpp"
    "$root/src/haard/ir/ir_opcode.cpp"
    "$root/src/haard/ir/ir_function.cpp"
    "$root/src/haard/ir/ir_program.cpp"
    "$root/src/haard/ir/ir_builder.cpp"
    "$root/src/haard/ir/ir_printer.cpp"
    "$root/src/haard/ir/ir_verifier.cpp"
    "$root/src/haard/ir/ir_flattener.cpp"
    "$root/src/haard/ir/ir_interpreter.cpp"
    "$root/src/haard/lowering/ir_lowerer.cpp"
    "$root/src/haard/lowering/function_lowerer.cpp"
)

rm -rf "$build"
mkdir -p "$build"

# a program here is one that may be wrong: no core, and it cannot fill the disk or
# hold the suite up
ulimit -c 0
ulimit -f 102400
limit=30

. "$root/tests/objects.sh"

if ! haard_objects "$root" "${sources[@]}"; then
    exit 1
fi

if ! err=$(g++ -std=c++20 -I"$root/src" -o "$build/lower" \
        ../lowering/lower.cpp "${HAARD_OBJECTS[@]}" 2>&1); then
    printf 'ERROR failed to build the interpreter\n%s\n' "$err"
    exit 1
fi

if ! err=$(make -s -C "$root/toolchain/hasm" 2>&1); then
    printf 'ERROR hasm does not build\n%s\n' "$err"
    exit 1
fi

passed=0
failed=0
known=0
: > "$build/reasons"

fail() {
    printf 'FAIL  %s\n' "$1"
    [ $# -ge 2 ] && printf '%s\n' "$2" | sed -n '1,20p' | sed 's/^/        /'
    failed=$((failed + 1))
}

# how a project's table and entry are named, the way tests/lowering/lower.cpp reads
# them, into 'options' and 'entry'
project() {
    local directory=$1

    options=()
    entry=myapp/main.hd

    [ -f "$directory/entry" ] && entry=$(head -1 "$directory/entry")

    if [ -f "$directory/table.tbl" ]; then
        options=(--roots "$directory/table.tbl")
    elif [ -f "$directory/roots.tbl" ]; then
        options=(--roots "$directory/roots.tbl")
    fi
}

# what a built program did, written the way a golden ends: '--- output:' when it
# printed something, then '--- exit: N'. It runs where it can leave files
ending_of() {
    local binary=$1 directory=$2 input=/dev/null printed code place

    [ -f "$directory/stdin" ] && input=$directory/stdin

    place=$(mktemp -d "$build/run.XXXXXX")
    printed=$(cd "$place" && timeout "$limit" "$binary" < "$input" 2>&1)
    code=$?
    rm -rf "$place"

    if [ -n "$printed" ]; then
        printf -- '--- output:\n%s\n' "$printed"
    fi

    printf -- '--- exit: %s' "$code"
}

# 'name', 'directory': builds it native into $build/$name.bin. 0 when it is built;
# 2 when it is not written yet, which is counted; 1 when something is wrong, said
native_build() {
    local label=$1 directory=$2 name=$3 err

    project "$directory"

    if ! err=$("$hdc" "${options[@]}" --emit-asm "$directory/$entry" 2>&1 > "$build/$name.s"); then
        if printf '%s' "$err" | grep -q 'does not write'; then
            printf '%s\n' "$err" | sed 's/^[^:]*: the native back end does not write //; s/ yet$//' >> "$build/reasons"
            known=$((known + 1))
            return 2
        fi

        fail "$label: hdc refused it" "$err"
        return 1
    fi

    if ! err=$("$hasm" "$runtime" "$build/$name.s" -o "$build/$name.bin" 2>&1); then
        # the one native that is decided to come last (record 0084)
        if printf '%s' "$err" | grep -q "the label '__pow' is not defined"; then
            printf "__pow\n" >> "$build/reasons"
            known=$((known + 1))
            return 2
        fi

        fail "$label: hasm refused what hdc wrote" "$err"
        return 1
    fi

    chmod +x "$build/$name.bin"
}

# ---- three ways, live

three_ways() {
    local label=$1 directory=$2 name=$3 err native cpp ir

    native_build "$label" "$directory" "$name"

    case $? in
        0) ;;
        2) fail "$label: a case of this suite has to be written" "$(tail -1 "$build/reasons")"; return ;;
        *) return ;;
    esac

    native=$(ending_of "$build/$name.bin" "$directory")

    if ! "$hdc" "${options[@]}" --emit-cpp "$directory/$entry" > "$build/$name.cpp" 2> "$build/$name.err"; then
        fail "$label: the C++ path refused it" "$(cat "$build/$name.err")"
        return
    fi

    if ! err=$(g++ -std=c++17 -funsigned-char -ffp-contract=off -ftrivial-auto-var-init=pattern -w \
            -o "$build/$name.cpp.bin" "$build/$name.cpp" 2>&1); then
        fail "$label: g++ refused it" "$err"
        return
    fi

    cpp=$(ending_of "$build/$name.cpp.bin" "$directory")
    ir=$(timeout "$limit" "$build/lower" "$directory" --run 2>&1)

    if [ "$cpp" != "$ir" ]; then
        fail "$label: the C++ and the IR disagree, so there is no answer" \
             "$(diff <(printf '%s\n' "$cpp") <(printf '%s\n' "$ir"))"
    elif [ "$native" != "$cpp" ]; then
        fail "$label: native ends otherwise" \
             "$(diff <(printf '%s\n' "$cpp") <(printf '%s\n' "$native"))"
    else
        passed=$((passed + 1))
    fi
}

for directory in cases/*/; do
    name=$(basename "$directory")
    three_ways "cases/$name" "$directory" "case-$name"
done

for type in i8 i16 i32 i64 u8 u16 u32 u64; do
    directory=$build/widths-$type
    mkdir -p "$directory/myapp"
    ./widths.sh "$type" > "$directory/myapp/main.hd"
    three_ways "widths/$type" "$directory" "width-$type"
done

# ---- the programs that are there already, against what their goldens say

against_golden() {
    local label=$1 directory=$2 name=$3 want=$4 got

    native_build "$label" "$directory" "$name"

    case $? in
        0) ;;
        *) return ;;
    esac

    got=$(ending_of "$build/$name.bin" "$directory")

    if [ "$got" != "$want" ]; then
        fail "$label: native ends otherwise than the C++" \
             "$(diff <(printf '%s\n' "$want") <(printf '%s\n' "$got"))"
    else
        printf 'PASS  %s\n' "$label"
        passed=$((passed + 1))
    fi
}

for directory in ../emitter/cases/*/; do
    name=$(basename "$directory")
    golden=../emitter/expected/$name.txt
    want=$(awk '/^--- (output|exit):/ { keep = 1 } keep' "$golden")

    # a case the C++ path itself refused has no answer to agree with
    [ -z "$want" ] && continue

    against_golden "emitter/$name" "$directory" "emitter-$name" "$want"
done

for directory in ../programs/cases/*/; do
    name=$(basename "$directory")
    golden=../programs/expected/$name.txt
    printed=$(awk '/^--- files:$/ { exit } { print }' "$golden")
    want="--- exit: $(tail -1 "$golden" | sed 's/^--- exit: //')"

    if [ -n "$printed" ]; then
        want="--- output:
$printed
$want"
    fi

    against_golden "programs/$name" "$directory" "programs-$name" "$want"
done

printf 'native: %d passed, %d failed, %d not written yet\n' "$passed" "$failed" "$known"

if [ "$known" -gt 0 ]; then
    sort "$build/reasons" | uniq -c | sort -rn | sed 's/^/    /'
fi

[ "$failed" -eq 0 ]
