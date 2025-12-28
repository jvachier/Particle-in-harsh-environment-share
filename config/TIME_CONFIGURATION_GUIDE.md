# Time Configuration Guide

## Quick Reference: Adjusting Simulation Time

### Understanding the Time Parameters

The simulation time is controlled by two key parameters in [simulation_parameters.h](simulation_parameters.h):

```cpp
double dt = 1e5;    // Time step size (seconds)
int nt = 15768;     // Number of time steps per iteration
```

### Time Step Size (dt)

- **Current value**: `dt = 1e5` seconds = **1.157 days per step**
- This is the physical time that advances with each computational step
- Smaller `dt` = more accuracy but longer computation time

### Number of Steps (nt)

- **Current value**: `nt = 15768` steps
- At `dt = 1e5`, this equals **~50 years** of simulation time
- Calculation: `15768 steps × 1.157 days/step = 18,244 days ≈ 50 years`

---

## Common Configuration Examples

### 1. Quick Testing (1 year simulation)

```cpp
double dt = 1e5;    // Keep same time step
int nt = 316;       // ~1 year (316 steps × 1.157 days = 365 days)
```

**Speedup**: ~50× faster than default
**Use case**: Testing changes, debugging, quick iterations

### 2. Medium Run (10 years)

```cpp
double dt = 1e5;    // Keep same time step
int nt = 3158;      // ~10 years
```

**Speedup**: ~5× faster than default
**Use case**: Intermediate validation

### 3. Default (50 years) - Current Setting

```cpp
double dt = 1e5;    // 1.157 days per step
int nt = 15768;     // ~50 years
```

**Use case**: Full production runs

### 4. Long Simulation (100 years)

```cpp
double dt = 1e5;    // Keep same time step
int nt = 31536;     // ~100 years
```

**Runtime**: ~2× slower than default
**Use case**: Long-term evolution studies

### 5. High Precision (50 years, smaller dt)

```cpp
double dt = 5e4;    // 0.579 days per step (half the default)
int nt = 31536;     // ~50 years (need 2× more steps)
```

**Runtime**: ~2× slower, but more accurate
**Use case**: When you need finer time resolution

---

## Quick Calculation Formula

To set a specific simulation duration in years:

```
Steps needed = (Years × 365.25 days) / (dt in days)
             = (Years × 365.25 × 86400) / dt

For dt = 1e5:
    Steps per year ≈ 316
    nt = 316 × (desired years)
```

### Examples:
- **1 year**: `nt = 316`
- **5 years**: `nt = 1580`
- **10 years**: `nt = 3160`
- **25 years**: `nt = 7900`
- **50 years**: `nt = 15800` (current default: 15768)
- **100 years**: `nt = 31600`

---

## Performance Estimates

Based on optimized implementation (50×50×1600 grid):

| Configuration | Time Steps | Simulation Time | Expected Runtime* |
|---------------|-----------|----------------|-------------------|
| 1 year test   | 316       | 1 year         | ~3 seconds        |
| 10 years      | 3,158     | 10 years       | ~30 seconds       |
| **50 years (default)** | **15,768** | **50 years** | **~2.4 min (pragma)** |
| 100 years     | 31,536    | 100 years      | ~4.8 minutes      |

\* Using optimized pragma implementation. Metal GPU is ~5× faster.

---

## How to Change Settings

### Option 1: Edit Configuration File (Recommended)

1. Open [simulation_parameters.h](simulation_parameters.h)
2. Find the `TimeParameters` struct (around line 51)
3. Change the `nt` value:
   ```cpp
   int nt = 316;  // Change to desired number of steps
   ```
4. Rebuild:
   ```bash
   cd implementation_pragma && make clean && make
   ```

### Option 2: Command Line Arguments

```bash
# Run with custom time steps (no recompile needed if supported)
./main_pragma.out --nt 316 --years 1
```

---

## Stability Considerations

### Time Step Stability (CFL Condition)

For numerical stability, the time step must satisfy:

```
dt ≤ (Δx²) / (2 × D_max)
```

Where:
- `Δx` = grid spacing (smallest of dx, dy, dz)
- `D_max` = maximum diffusion coefficient

**Current grid**: `Δz = 0.05` (smallest)
**Current `D_c`**: `1e-10`
**Maximum stable `dt`**: ~1.25e7 seconds

✅ **Current `dt = 1e5` is well within stable range** (125× safety margin)

### When to Reduce `dt`:

- Increasing diffusion coefficients
- Refining the grid (smaller Δx, Δy, Δz)
- Observing numerical instabilities (NaN, oscillations)

### When You Can Increase `dt`:

- Coarser grids
- Smaller diffusion coefficients
- Less stiff reaction terms
- **Always verify stability by checking output**

---

## Example: Testing Workflow

For development and testing, use this efficient workflow:

1. **Initial test** (1 year):
   ```cpp
   int nt = 316;  // Quick verification (~3 seconds)
   ```

2. **Medium validation** (10 years):
   ```cpp
   int nt = 3158;  // Check trends (~30 seconds)
   ```

3. **Production run** (50 years):
   ```cpp
   int nt = 15768;  // Full simulation (~2.4 minutes)
   ```

This approach lets you iterate quickly during development and only run expensive full simulations when needed.

---

## Summary

**For quick testing**: Set `nt = 316` (1 year, ~3 seconds)
**For production**: Keep `nt = 15768` (50 years, ~2.4 minutes)
**For long runs**: Set `nt = 31536` (100 years, ~5 minutes)

Remember to rebuild after changing simulation_parameters.h:
```bash
make clean && make
```
