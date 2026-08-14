CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O3 -march=native -Wall -Wextra -Iinclude
LDFLAGS  ?= -pthread

.PHONY: all test clean

all: build/mcre

build/mcre: src/main.cpp $(wildcard include/mcre/*.hpp)
	@mkdir -p build data
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

build/test_core: tests/test_core.cpp $(wildcard include/mcre/*.hpp)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

test: build/test_core
	./build/test_core

clean:
	rm -rf build
