# ============================================================================
# Pragma SIMD Tests Makefile
# ============================================================================

# Detect platform for compiler selection
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	CXX = clang++
	CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Wno-unused-parameter -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
	LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp
else
	CXX = g++-13
	CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wno-unused-parameter -fopenmp
	LDFLAGS = -fopenmp
endif

INCLUDES = -I../implementation_pragma/headers -I../config

# Test binaries
TEST_PRAGMA = test_pragma.bin

# Implementation source files needed for testing
IMPL_SRC = ../implementation_pragma/concentration_field_density.cpp

# ============================================================================
# Build Tests
# ============================================================================
all: $(TEST_PRAGMA)

$(TEST_PRAGMA): test_pragma.cpp $(IMPL_SRC)
	@echo "Compiling pragma tests..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) test_pragma.cpp $(IMPL_SRC) -o $@ $(LDFLAGS)
	@echo "✓ Test binary created: $(TEST_PRAGMA)"

# ============================================================================
# Run Tests
# ============================================================================
test: $(TEST_PRAGMA)
	@echo "Running pragma tests..."
	@./$(TEST_PRAGMA)
	@echo "Cleaning up test binary..."
	@rm -f $(TEST_PRAGMA)
	@echo "✓ Test binary removed"

# ============================================================================
# Clean
# ============================================================================
clean:
	@echo "Cleaning test artifacts..."
	@rm -f $(TEST_PRAGMA)
	@echo "✓ Test artifacts cleaned"

.PHONY: all test clean
