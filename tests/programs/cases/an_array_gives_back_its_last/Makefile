# How a Haard program becomes a binary, written out so it can be read.
#
#   make          build ./app
#   make run      build it and run it
#   make cpp      stop after the C++, to look at it
#   make clean
#
# Two steps, and that is the whole of it today:
#
#   1. hdc reads the ENTRY file, follows every import it reaches through
#      ROOTS, checks the whole program, and writes it as ONE C++ file.
#      Every module of the program is in that one file -- there is no
#      per-module object and no linking of Haard against Haard (record 0025).
#
#   2. a C++ compiler turns that file into an executable.
#
# There is no build system inside hdc: it is told where the roots are and it
# searches nowhere else (record 0010). ROOTS is the whole of what a build
# system would have to generate.

# where hdc is. Override it if your build tree is somewhere else:
#   make HDC=/path/to/hdc
HDC ?= ../../../../build/hdc

ROOTS ?= roots.tbl
ENTRY ?= myapp/main.hd

# the C++ standard the emitted file needs. It uses nothing exotic -- cstdint
# for the sized builtins and cstdio for record 0030's eight native functions
CXX ?= g++
CXXFLAGS ?= -std=c++17 -w

# every Haard source of every root, so editing any of them rebuilds
SOURCES := $(wildcard */*.hd)

.PHONY: all run cpp clean

all: app

# step 1: the whole program as one C++ file
main.cpp: $(SOURCES) $(ROOTS)
	$(HDC) --roots $(ROOTS) --emit-cpp $(ENTRY) > main.cpp

cpp: main.cpp

# step 2: an ordinary C++ compile, and nothing else is needed
app: main.cpp
	$(CXX) $(CXXFLAGS) -o app main.cpp

# the program writes its images into the working directory and reads them
# back, so its exit status is the number of checks that failed
run: app
	./app

# 'hdc file.hd' without --roots checks ONE file and does not follow its
# imports, which is not enough for this program -- it is here to show the
# difference
check: $(SOURCES) $(ROOTS)
	$(HDC) --roots $(ROOTS) $(ENTRY)

clean:
	rm -f main.cpp app *.ppm
