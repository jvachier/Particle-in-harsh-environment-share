# Central Makefile for Particle Simulation Project
# All build rules consolidated in this single file

# Detect platform for compiler selection
UNAME_S := $(shell uname -s)

# ============================================================================
# Pragma SIMD Implementation Configuration
# ============================================================================
ifeq ($(UNAME_S),Darwin)
	PRAGMA_CC = clang++ -O3 -std=c++17 -stdlib=libc++
	PRAGMA_CFLAGS = -Wall -g -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include
	PRAGMA_LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp
else
	PRAGMA_CC = g++-14 -O3 -std=c++17
	PRAGMA_CFLAGS = -Wall -g -fopenmp -fopenmp-simd
	PRAGMA_LDFLAGS =
endif

PRAGMA_DIR = implementation_pragma
PRAGMA_OBJS = $(PRAGMA_DIR)/main.o $(PRAGMA_DIR)/oldtonew.o \
              $(PRAGMA_DIR)/concentration_field_density.o $(PRAGMA_DIR)/concentration_field.o \
              $(PRAGMA_DIR)/initialization.o $(PRAGMA_DIR)/initialization_fcts.o \
              $(PRAGMA_DIR)/memory_allocation.o $(PRAGMA_DIR)/delocate_memory.o \
              $(PRAGMA_DIR)/print_position.o $(PRAGMA_DIR)/print_initial_position.o

# ============================================================================
# Metal GPU Implementation Configuration
# ============================================================================
ifeq ($(UNAME_S),Darwin)
	METAL_CC = clang++ -O3 -std=c++17 -stdlib=libc++
	METAL_CFLAGS = -Wall -g -Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include -Wno-pass-failed
	METAL_LDFLAGS = -L/opt/homebrew/opt/libomp/lib -lomp -framework Metal -framework Foundation
	METAL_OBJC_COMPILER = clang++ -x objective-c++ -O3 -std=c++17 -stdlib=libc++
	METAL_SPECIFIC_OBJS = $(METAL_DIR)/concentration_field_density_metal.o $(METAL_DIR)/concentration_field_metal.o $(METAL_DIR)/gpu_pipeline_metal.o
else
	METAL_CC = g++-14 -O3 -std=c++17
	METAL_CFLAGS = -Wall -g -fopenmp -fopenmp-simd
	METAL_LDFLAGS =
	METAL_OBJC_COMPILER =
	METAL_SPECIFIC_OBJS =
endif

METAL_DIR = implementation_metal
METAL_OBJS = $(METAL_DIR)/main.o $(METAL_DIR)/oldtonew.o \
             $(METAL_DIR)/concentration_field_density.o $(METAL_SPECIFIC_OBJS) \
             $(METAL_DIR)/concentration_field.o $(METAL_DIR)/initialization.o \
             $(METAL_DIR)/initialization_fcts.o $(METAL_DIR)/memory_allocation.o \
             $(METAL_DIR)/delocate_memory.o $(METAL_DIR)/print_position.o \
             $(METAL_DIR)/print_initial_position.o

# ============================================================================
# Targets
# ============================================================================
.PHONY: all pragma metal clean test benchmark help compare run-pragma run-metal rebuild

# Default target
all: pragma metal

# ============================================================================
# Pragma SIMD Build Rules
# ============================================================================
pragma: $(PRAGMA_DIR)/main_pragma.out

$(PRAGMA_DIR)/main_pragma.out: $(PRAGMA_OBJS)
	@echo "Linking Pragma SIMD executable..."
	$(PRAGMA_CC) $(PRAGMA_CFLAGS) -o $@ $(PRAGMA_OBJS) $(PRAGMA_LDFLAGS)

$(PRAGMA_DIR)/main.o: $(PRAGMA_DIR)/main.cpp
	@echo "Compiling Pragma main.cpp..."
	$(PRAGMA_CC) $(PRAGMA_CFLAGS) -c $< -o $@

$(PRAGMA_DIR)/%.o: $(PRAGMA_DIR)/%.cpp
	@echo "Compiling Pragma $<..."
	$(PRAGMA_CC) $(PRAGMA_CFLAGS) -c $< -o $@

# ============================================================================
# Metal GPU Build Rules
# ============================================================================
metal: $(METAL_DIR)/main_metal.out

$(METAL_DIR)/main_metal.out: $(METAL_OBJS)
	@echo "Linking Metal GPU executable..."
	$(METAL_CC) $(METAL_CFLAGS) -o $@ $(METAL_OBJS) $(METAL_LDFLAGS)

$(METAL_DIR)/main.o: $(METAL_DIR)/main.cpp
	@echo "Compiling Metal main.cpp..."
	$(METAL_CC) $(METAL_CFLAGS) -c $< -o $@

$(METAL_DIR)/%.o: $(METAL_DIR)/%.cpp
	@echo "Compiling Metal $<..."
	$(METAL_CC) $(METAL_CFLAGS) -c $< -o $@

ifeq ($(UNAME_S),Darwin)
$(METAL_DIR)/concentration_field_density_metal.o: $(METAL_DIR)/concentration_field_density_metal.mm
	@echo "Compiling Metal GPU kernel (concentration_field_density_metal.mm)..."
	$(METAL_OBJC_COMPILER) $(METAL_CFLAGS) -c $< -o $@

$(METAL_DIR)/concentration_field_metal.o: $(METAL_DIR)/concentration_field_metal.mm
	@echo "Compiling Metal GPU kernel (concentration_field_metal.mm)..."
	$(METAL_OBJC_COMPILER) $(METAL_CFLAGS) -c $< -o $@

$(METAL_DIR)/gpu_pipeline_metal.o: $(METAL_DIR)/gpu_pipeline_metal.mm
	@echo "Compiling Metal GPU pipeline (gpu_pipeline_metal.mm)..."
	$(METAL_OBJC_COMPILER) $(METAL_CFLAGS) -c $< -o $@
endif

# ============================================================================
# Clean and Maintenance
# ============================================================================
clean:
	@echo "Cleaning all builds..."
	@rm -f $(PRAGMA_DIR)/*.o $(PRAGMA_DIR)/*.out
	@rm -f $(METAL_DIR)/*.o $(METAL_DIR)/*.out
	@rm -f tests/*.o tests/*.out
	@rm -f *.log performance_summary.txt
	@echo "Clean complete"

# ============================================================================
# Tests and Benchmarks
# ============================================================================
test:
	@echo "Running pragma tests..."
	@cd tests && $(MAKE) test

benchmark: all
	@echo "Running performance comparison..."
	@./compare_performance.sh

compare: benchmark

# ============================================================================
# Run Targets
# ============================================================================
run-pragma: pragma
	@echo "Running Pragma SIMD implementation..."
	@cd $(PRAGMA_DIR) && ./main_pragma.out

run-metal: metal
	@echo "Running Metal GPU implementation..."
	@cd $(METAL_DIR) && ./main_metal.out

# ============================================================================
# Rebuild and Help
# ============================================================================
rebuild: clean all

help:
	@echo "════════════════════════════════════════════════════════════════"
	@echo "  Particle Simulation - Centralized Makefile"
	@echo "════════════════════════════════════════════════════════════════"
	@echo ""
	@echo "Build Targets:"
	@echo "  make all        - Build both implementations (default)"
	@echo "  make pragma     - Build pragma SIMD implementation only"
	@echo "  make metal      - Build Metal GPU implementation only"
	@echo "  make clean      - Clean all build artifacts"
	@echo "  make rebuild    - Clean and rebuild everything"
	@echo ""
	@echo "Run Targets:"
	@echo "  make run-pragma - Build and run pragma implementation"
	@echo "  make run-metal  - Build and run metal implementation"
	@echo ""
	@echo "Testing and Benchmarking:"
	@echo "  make test       - Run correctness tests"
	@echo "  make benchmark  - Run performance comparison (alias: 'compare')"
	@echo ""
	@echo "Configuration (NO RECOMPILATION NEEDED!):"
	@echo "  Runtime configuration via command-line arguments:"
	@echo "    ./main_pragma.out --years 10 --beta -1e-10 --threads 8"
	@echo "    ./main_metal.out --years 10 --nz 2000 --help"
	@echo ""
	@echo "  Or edit config/simulation_parameters.h for default values"
	@echo ""
	@echo "════════════════════════════════════════════════════════════════"
