# How the bootstrap becomes binaries, written out so it can be read.
#
#   make           build build/dump_tokens and build/parse_and_print
#   make test      build them and run them against the C++ compiler's goldens
#   make cpp       stop after the C++, to look at it
#   make table     build with roots.tbl instead of the manifest, and check the
#                  two give the same programs
#   make clean
#
# Two steps, the same two every Haard program takes: hdc writes the whole
# program as ONE C++ file, and a C++ compiler turns that into a binary. There
# is no build system inside hdc -- the manifest says where everything is, and
# hdc searches nowhere else (records 0010 and 0044).
#
# Each tool is a program of its own: a file in tools/ with a 'main', and every
# module it imports. They share the modules and nothing else.

# where hdc is. Override it if your build tree is somewhere else:
#   make HDC=/path/to/hdc
HDC ?= ../build/hdc

CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -w

# every Haard source of this package and the library it stands on, so editing
# any of them rebuilds -- and hdc itself, since a new compiler is a new program
SOURCES := $(wildcard */*.hd) $(wildcard ../std/*.hd) haard.pkg

TOOLS := dump_tokens parse_and_print

.PHONY: all test cpp table clean

all: $(addprefix build/,$(TOOLS))

# step 1: the whole program as one C++ file. The manifest is found by hdc on
# its own, beside the entry file or above it; it is named here to be read
build/%.cpp: tools/%.hd $(SOURCES) $(HDC)
	@mkdir -p build
	$(HDC) --pkg haard.pkg --emit-cpp $< > $@

# step 2: an ordinary C++ compile, and nothing else is needed
build/%: build/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# make does not keep a generated C++ file only because a binary was built from
# it, and this one is worth reading
.PRECIOUS: build/%.cpp

cpp: $(addprefix build/,$(addsuffix .cpp,$(TOOLS)))

test: all
	tests/run.sh

# The manifest and the table are two spellings of one build. This writes each
# program both ways and fails if they differ, which is what keeps roots.tbl
# from going stale
table: $(SOURCES) $(HDC) roots.tbl
	@mkdir -p build
	@for tool in $(TOOLS); do \
	    $(HDC) --roots roots.tbl --emit-cpp tools/$$tool.hd > build/$$tool.by_table.cpp || exit 1; \
	    $(HDC) --pkg haard.pkg --emit-cpp tools/$$tool.hd > build/$$tool.by_manifest.cpp || exit 1; \
	    diff -u build/$$tool.by_manifest.cpp build/$$tool.by_table.cpp || exit 1; \
	done
	@echo "roots.tbl and haard.pkg build the same programs"

clean:
	rm -rf build
