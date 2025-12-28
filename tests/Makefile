# Tests Makefile - Pragma SIMD tests only (CI-compatible)
# Depends on centralized root Makefile

# Detect platform
UNAME_S := $(shell uname -s)

# Compiler settings
ifeq ($(UNAME_S),Darwin)
	CC = clang++ -O3 -std=c++17 -stdlib=libc++
	CFLAGS = -Wall -g -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
	LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp
else
	CC = g++-14 -O3 -std=c++17
	CFLAGS = -Wall -g -fopenmp -fopenmp-simd
	LDFLAGS =
endif

PRAGMA_DIR = ../implementation_pragma

# Object files needed from pragma implementation (built by root Makefile)
PRAGMA_OBJS = $(PRAGMA_DIR)/concentration_field_density.o

.PHONY: all test clean clean-all

all: test

# Ensure pragma implementation is built first
prereq:
	@echo "Building pragma implementation..."
	@cd .. && $(MAKE) pragma

# Pragma tests (CI-compatible, no Metal dependency)
test: prereq test_pragma.out
	./test_pragma.out

test_pragma.out: test_pragma.o $(PRAGMA_OBJS)
	$(CC) $(CFLAGS) -o test_pragma.out test_pragma.o $(PRAGMA_OBJS) $(LDFLAGS)

test_pragma.o: test_pragma.cpp
	$(CC) $(CFLAGS) -I$(PRAGMA_DIR) -c test_pragma.cpp

clean:
	rm -f *.o *.out

clean-all: clean
	@cd .. && $(MAKE) clean
