# Compiles the compiler's sources once for the whole test run, instead of once
# per suite.
#
# Every suite used to hand its twenty odd sources straight to g++, which is the
# simplest thing that works and is what these scripts want to be: no build
# system, no generated makefile, nothing to keep in step with CMakeLists.txt.
# The cost was that 'make check' compiled the same files eleven times, one file
# at a time, and took minutes.
#
# So: one object directory shared by all the suites, one compile per source per
# change, and the compiles run at once instead of in a row. The second suite of
# a 'make check' compiles nothing at all, and a run with nothing edited
# compiles nothing at all either.
#
# What decides that a source has to be compiled again is g++'s own answer.
# '-MMD' writes a '.d' file next to the object listing every header the source
# actually included, so a header nobody can see from the file list still
# rebuilds what includes it. Deciding by hand -- a file list, a stamp, the
# newest thing under src/ -- is either wrong or rebuilds everything, and this
# is neither.
#
# Usage, from a suite's run.sh:
#
#   . "$root/tests/objects.sh"
#   haard_objects "$root" "${sources[@]}" || exit 1
#   g++ -std=c++20 -I"$root/src" -o "$build/case" case.cpp "${HAARD_OBJECTS[@]}"
#
# The objects come back in HAARD_OBJECTS, in the order they were asked for.
# Dependencies: g++ and bash, the same two the suites already needed.

HAARD_OBJECT_DIR=${TMPDIR:-/tmp}/haard-test-objects

# the object a source compiles to. The path is flattened into the name, so two
# 'ast.cpp' in two directories cannot collide
haard_object_of() {
    local source=$1

    source=${source#*/src/}
    source=${source//\//_}

    printf '%s/%s.o' "$HAARD_OBJECT_DIR" "${source%.cpp}"
}

# whether this object has to be built: it is missing, or something it was
# built from is newer than it
haard_stale() {
    local object=$1
    local source=$2
    local depfile=${object%.o}.d
    local prerequisite

    if [ ! -f "$object" ] || [ ! -f "$depfile" ]; then
        return 0
    fi

    if [ "$source" -nt "$object" ]; then
        return 0
    fi

    # the '.d' is a make rule: a target, a colon, and the prerequisites spread
    # over lines that end in a backslash. Only the prerequisites are wanted
    for prerequisite in $(sed -e 's/^[^:]*://' -e 's/\\$//' "$depfile"); do
        if [ ! -e "$prerequisite" ] || [ "$prerequisite" -nt "$object" ]; then
            return 0
        fi
    done

    return 1
}

haard_objects() {
    local root=$1
    shift

    local jobs
    local source
    local object
    local built=()
    local failed=0

    jobs=$(nproc 2>/dev/null) || jobs=4

    mkdir -p "$HAARD_OBJECT_DIR"

    HAARD_OBJECTS=()

    for source in "$@"; do
        object=$(haard_object_of "$source")
        HAARD_OBJECTS+=("$object")

        if ! haard_stale "$object" "$source"; then
            continue
        fi

        # 'wait -n' gives the slot back as soon as any one compile finishes,
        # so the last file does not hold the next batch up
        while [ "$(jobs -rp | wc -l)" -ge "$jobs" ]; do
            wait -n 2>/dev/null || break
        done

        rm -f "$object.ok"

        # the exit code cannot be read afterwards -- 'wait -n' above has
        # already reaped some of these -- so a compile that worked leaves a
        # file behind saying so
        (
            if g++ -std=c++20 -I"$root/src" -MMD -MF "${object%.o}.d" \
                    -c "$source" -o "$object" 2> "$object.err"; then
                : > "$object.ok"
            fi
        ) &

        built+=("$object")
    done

    wait

    for object in "${built[@]}"; do
        if [ ! -f "$object.ok" ]; then
            cat "$object.err" >&2
            failed=1
        fi
    done

    return $failed
}
