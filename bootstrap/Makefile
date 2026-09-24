# How the bootstrap becomes a binary, written out so it can be read.
#
#   make           build build/dump_tokens
#   make test      build it and run it against the C++ scanner's goldens
#   make cpp       stop after the C++, to look at it
#   make table     build with roots.tbl instead of the manifest, and check the
#                  two give the same program
#   make clean
#
# Two steps, the same two every Haard program takes: hdc writes the whole
# program as ONE C++ file, and a C++ compiler turns that into a binary. There
# is no build system inside hdc -- the manifest says where everything is, and
# hdc searches nowhere else (records 0010 and 0044).

# where hdc is. Override it if your build tree is somewhere else:
#   make HDC=/path/to/hdc
HDC ?= ../build/hdc

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -w

# every Haard source of this package and the library it stands on, so editing
# any of them rebuilds -- and hdc itself, since a new compiler is a new program
SOURCES := $(wildcard */*.hd) $(wildcard ../std/*.hd) haard.pkg

.PHONY: all test cpp table clean

all: build/dump_tokens

# step 1: the whole program as one C++ file. The manifest is found by hdc on
# its own, beside the entry file or above it; it is named here to be read
build/dump_tokens.cpp: $(SOURCES) $(HDC)
	@mkdir -p build
	$(HDC) --pkg haard.pkg --emit-cpp tools/dump_tokens.hd > $@

# step 2: an ordinary C++ compile, and nothing else is needed
build/dump_tokens: build/dump_tokens.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

cpp: build/dump_tokens.cpp

test: build/dump_tokens
	tests/run.sh

# The manifest and the table are two spellings of one build. This writes the
# program both ways and fails if they differ, which is what keeps roots.tbl
# from going stale
table: $(SOURCES) $(HDC) roots.tbl
	@mkdir -p build
	$(HDC) --roots roots.tbl --emit-cpp tools/dump_tokens.hd > build/by_table.cpp
	$(HDC) --pkg haard.pkg --emit-cpp tools/dump_tokens.hd > build/by_manifest.cpp
	diff -u build/by_manifest.cpp build/by_table.cpp
	@echo "roots.tbl and haard.pkg build the same program"

clean:
	rm -rf build
