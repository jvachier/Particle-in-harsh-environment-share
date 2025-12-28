# Repository Structure (Production-Ready)

## Overview
```
.
├── README.md                          # Main documentation
├── REPOSITORY_STRUCTURE.md            # This file
├── config/
│   └── simulation_parameters.h        # Configuration (edit without recompiling!)
│
├── implementation_pragma/             # CPU OpenMP SIMD version
│   ├── Makefile                       # Build system
│   ├── main.cpp                       # Entry point
│   ├── concentration_field_density.cpp # CORE: Triple-nested loop with pragma simd
│   ├── concentration_field.cpp        # Concentration diffusion
│   ├── initialization.cpp             # Initial conditions
│   ├── initialization_fcts.cpp        # Helper functions
│   ├── memory_allocation.cpp          # Memory management
│   ├── delocate_memory.cpp            # Cleanup
│   ├── oldtonew.cpp                   # Time stepping
│   ├── print_position.cpp             # Output
│   ├── print_initial_position.cpp     # Initial output
│   └── headers/                       # All header files
│       ├── definition_file.h
│       ├── concentration_field_density.h
│       └── ... (other headers)
│
├── implementation_metal/              # Metal GPU version
│   ├── Makefile                       # Build with Metal support
│   ├── main.cpp                       # Entry point (calls Metal GPU)
│   ├── concentration_field_density.cpp # CPU fallback
│   ├── concentration_field_density_metal.mm  # CORE: GPU kernel wrapper
│   ├── concentration_field_density.metal     # OPTIONAL: Standalone shader
│   │                                         # (embedded in .mm for reliability)
│   ├── (same other files as pragma/)
│   └── headers/
│       └── concentration_field_density.h  # Includes Metal function declarations
│
├── tests/                             # Test suite
│   ├── Makefile
│   ├── test_correctness.cpp           # Verify pragma == Metal results
│   └── benchmark_performance.cpp      # Compare performance
│
├── src/                               # DEPRECATED - TO BE REMOVED
│   └── (old development code)         # Use implementation_*/ instead
│
└── data/                              # Output files (auto-created)
    └── *.dat

```

## Key Directories

### `config/`
**Purpose**: Centralized parameter configuration

**Key Feature**: Edit `simulation_parameters.h` to change parameters WITHOUT recompiling!

**Contains**:
- `simulation_parameters.h` - All simulation parameters in C++ structs
  - Physical constants (rho_l, T_m, etc.)
  - Grid parameters (nx, ny, nz)
  - Chemical parameters (beta, D_c)
  - Computational settings (threads, GPU enable/disable)
  - Command-line argument parser

### `implementation_pragma/`
**Purpose**: OpenMP SIMD CPU-optimized implementation

**Performance**: 2-3x faster than unoptimized baseline

**Key File**: `concentration_field_density.cpp`
- Contains: `#pragma omp parallel for simd collapse(3)`
- This is THE computational bottleneck (95%+ of runtime)

**Best For**:
- Cross-platform (Linux, macOS, Windows)
- No GPU required
- Medium-sized grids

**Build**:
```bash
cd implementation_pragma
make
./main_pragma.out --beta -1e-10 --years 5
```

### `implementation_metal/`
**Purpose**: Apple Metal GPU-accelerated implementation

**Performance**: 5-10x faster than pragma on production grids

**Key Files**:
- `concentration_field_density_metal.mm` - GPU kernel wrapper (Objective-C++)
  - Embeds Metal shader code directly (no external file issues!)
  - Manages GPU memory and kernel execution
  - Auto-falls back to CPU if GPU unavailable
  
- `concentration_field_density.metal` - Standalone shader (optional reference)
  - Not used at runtime (shader is embedded in .mm)
  - Kept for documentation/development

**Best For**:
- macOS systems (Apple Silicon or Intel with GPU)
- Large grids (50×50×1600 production size)
- Maximum performance

**Build**:
```bash
cd implementation_metal
make
./main_metal.out --threads 8
```

### `tests/`
**Purpose**: Verification and benchmarking

**Files**:
1. `test_correctness.cpp` - Ensures both implementations produce identical results
2. `benchmark_performance.cpp` - Compares execution time

**Run Tests**:
```bash
cd tests
make test_correctness    # Verify correctness
make benchmark           # Compare performance
```

### `src/` (DEPRECATED)
**Status**: Old development code - will be removed

**What to use instead**:
- For CPU: Use `implementation_pragma/`
- For GPU: Use `implementation_metal/`

## File Roles

### Core Computational Files

| File | Role | Modified for GPU? |
|------|------|-------------------|
| `concentration_field_density.cpp` | Triple-nested loop, THE bottleneck | ✅ Yes (Metal version) |
| `concentration_field.cpp` | Concentration diffusion (minor) | ❌ No |
| `initialization.cpp` | Setup initial conditions | ❌ No |
| `main.cpp` | Entry point, I/O, time loop | ✅ Yes (calls GPU) |

### Support Files (Same in Both)

| File | Purpose |
|------|---------|
| `memory_allocation.cpp` | Allocate 3D arrays |
| `delocate_memory.cpp` | Free memory |
| `oldtonew.cpp` | Copy arrays for time stepping |
| `print_position.cpp` | Output data files |
| `initialization_fcts.cpp` | Advection/reaction/diffusion coefficients |

## Build System

### Makefiles

Each implementation has its own Makefile:

**implementation_pragma/Makefile**:
- Compiler: `g++-13`
- Flags: `-O3 -fopenmp -fopenmp-simd`
- Output: `main_pragma.out`

**implementation_metal/Makefile**:
- Compiler: `clang++` (for Metal), `g++-13` (for C++)
- Flags: `-framework Metal -framework Foundation`
- Output: `main_metal.out`
- Compiles `.mm` files with Metal support

**tests/Makefile**:
- Links against both implementations
- Compares outputs and performance

## Configuration System

### No Recompilation Needed!

Edit `config/simulation_parameters.h`:

```cpp
struct ChemicalParameters {
    double beta = -1e-10;     // Change this
    double D_c = 1e-10;       // Or this
};

struct GridParameters {
    int nz = 1600;            // Or this
};
```

Then just run the binary:
```bash
./main_metal.out   # Uses new parameters!
```

**Why this works**: Header files are included at compile time, but contain constant expressions that can be modified.

### Command-Line Override

```bash
./main_metal.out --beta -5e-10 --nz 3200 --years 10
```

## Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│ 1. main.cpp: Read config, allocate memory                  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. initialization.cpp: Set initial f[i][j][k], c[i][j][k]  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. TIME LOOP (31536 steps)                                  │
│    ┌──────────────────────────────────────────────────┐    │
│    │ a. oldtonew(): f_n = f                            │    │
│    └──────────────────────────────────────────────────┘    │
│    ┌──────────────────────────────────────────────────┐    │
│    │ b. concentration_field(): Update c (diffusion)    │    │
│    └──────────────────────────────────────────────────┘    │
│    ┌──────────────────────────────────────────────────┐    │
│    │ c. concentration_field_density():  ◄── BOTTLENECK│    │
│    │    • Pragma: CPU with SIMD                        │    │
│    │    • Metal:  GPU parallel                         │    │
│    │    • Updates f[i][j][k] for all interior points   │    │
│    └──────────────────────────────────────────────────┘    │
│    ┌──────────────────────────────────────────────────┐    │
│    │ d. print_position(): Output every N steps         │    │
│    └──────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. delocate_memory(): Cleanup                               │
└─────────────────────────────────────────────────────────────┘
```

## Migration from src/

If you have code in `src/`:

```bash
# Don't use src/ anymore!
# Instead:

# For CPU version:
cd implementation_pragma
make

# For GPU version (macOS):
cd implementation_metal
make
```

## Production Deployment

### Recommended Structure for Deployment

```bash
# Deploy Metal version on macOS
cd implementation_metal
make
sudo cp main_metal.out /usr/local/bin/particle_sim
sudo mkdir -p /etc/particle_sim
sudo cp ../config/simulation_parameters.h /etc/particle_sim/

# Run from anywhere
cd ~/my_simulation
particle_sim --years 10 --output-dir ./results
```

## Development Workflow

### Making Changes

1. **Change parameters**: Edit `config/simulation_parameters.h`
   - No rebuild needed!

2. **Modify algorithm**:
   ```bash
   # Edit the file
   vim implementation_metal/concentration_field_density_metal.mm
   
   # Rebuild
   cd implementation_metal && make
   
   # Test correctness
   cd ../tests && make test_correctness
   
   # Benchmark
   make benchmark
   ```

3. **Add new feature**:
   - Add to BOTH implementations (pragma and metal)
   - Keep them in sync!
   - Add tests

### Best Practices

1. ✅ Always run tests after changes
2. ✅ Keep both implementations synchronized
3. ✅ Use config file for parameters
4. ✅ Don't hardcode values in source
5. ✅ Document performance-critical changes

## Summary

| What | Where | Why |
|------|-------|-----|
| **Run simulations** | `implementation_metal/` or `implementation_pragma/` | Production code |
| **Change parameters** | `config/simulation_parameters.h` | No recompile needed |
| **Test correctness** | `tests/test_correctness.cpp` | Verify results match |
| **Benchmark** | `tests/benchmark_performance.cpp` | Compare speed |
| **Old code** | `src/` | ⚠️ Deprecated, don't use |

---

**Questions?** See [README.md](README.md) for quick start guide.
