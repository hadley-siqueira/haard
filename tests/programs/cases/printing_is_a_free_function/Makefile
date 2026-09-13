# How a Haard program becomes a binary, written out so it can be read.
#
#   make          build ./app
#   make run      build it and run it
#   make cpp      stop after the C++, to look at it
#   make clean

HDC ?= ../../../../build/hdc

ROOTS ?= roots.tbl
ENTRY ?= myapp/main.hd

CXX ?= g++
CXXFLAGS ?= -std=c++17 -w

SOURCES := $(wildcard */*.hd)

.PHONY: all run cpp clean

all: app

main.cpp: $(SOURCES) $(ROOTS)
	$(HDC) --roots $(ROOTS) --emit-cpp $(ENTRY) > main.cpp

cpp: main.cpp

app: main.cpp
	$(CXX) $(CXXFLAGS) -o app main.cpp

# what it prints IS the check here: the golden holds every line
run: app
	./app

clean:
	rm -f main.cpp app
