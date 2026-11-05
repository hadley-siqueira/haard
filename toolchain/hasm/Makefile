# How hasm becomes a binary: the two steps every Haard program takes today.
#
#   make           build build/hasm
#   make test      build it and run tests/run.sh
#   make clean
#
# 'make HDC=/path/to/hdc' when the compiler is somewhere else.
HDC ?= ../../build/hdc

# the three flags are the language's rules, which a C++ compiler leaves open
# (records 0081 and 0083)
CXX ?= g++
CXXFLAGS ?= -std=c++17 -funsigned-char -ffp-contract=off -ftrivial-auto-var-init=pattern -O2 -w

SOURCES := $(wildcard *.hd) $(wildcard ../../std/*.hd) haard.pkg

.PHONY: all test clean

all: build/hasm

build/hasm.cpp: $(SOURCES) $(HDC)
	@mkdir -p build
	$(HDC) --pkg haard.pkg --emit-cpp hasm.hd > $@

build/hasm: build/hasm.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

.PRECIOUS: build/hasm.cpp

test: all
	tests/run.sh

clean:
	rm -rf build
