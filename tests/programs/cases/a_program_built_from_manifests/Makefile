# How a Haard program is built **from its manifest**, written out so it can be
# read. Record 0044.
#
#   make          build ./app
#   make run      build it and run it
#   make cpp      stop after the C++, to look at it
#   make clean
#
# The difference from every other case here is that there is **no flag**, and
# no roots table either. 'hdc' looks for the nearest 'haard.pkg' at or above
# the file it was given -- 'myapp/main.hd', so 'myapp/haard.pkg' -- follows its
# path dependencies to 'shapes' and 'std', reads their manifests too, and
# builds the table itself. Nothing below that changed: record 0010 still
# decides what the compiler is handed.
#
# The 'check' target below writes '--pkg' out, which is what to do when the
# manifest is somewhere the search would not reach.

HDC ?= ../../../../build/hdc

PKG ?= myapp/haard.pkg
ENTRY ?= myapp/main.hd

CXX ?= g++
CXXFLAGS ?= -std=c++17 -w

SOURCES := $(wildcard */*.hd) $(wildcard */haard.pkg)

.PHONY: all run cpp clean

all: app

main.cpp: $(SOURCES)
	$(HDC) --emit-cpp $(ENTRY) > main.cpp

cpp: main.cpp

app: main.cpp
	$(CXX) $(CXXFLAGS) -o app main.cpp

run: app
	./app

# and the same program checked without being emitted
check: $(SOURCES)
	$(HDC) --pkg $(PKG) $(ENTRY)

clean:
	rm -f main.cpp app
