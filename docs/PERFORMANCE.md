# Performance Comparison Guide

Quick guide to benchmarking Pragma SIMD vs Metal GPU implementations.

## Quick Start

```bash
# Build and run performance comparison
./build_all.sh compare
```

This will:
1. Clean and rebuild both implementations
2. Run pragma SIMD version with timing
3. Run Metal GPU version with timing
4. Display summary and save full logs

## What Gets Measured

The `compare` command uses macOS `/usr/bin/time -l` which provides:

- **Real time**: Total wall-clock time
- **User time**: CPU time in user space
- **System time**: CPU time in kernel space
- **Memory usage**: Peak memory consumption
- **Metal GPU initialization**: First-time GPU setup

## Expected Results

For a **50×50×1600** grid with **1 year simulation**:

| Implementation | Expected Time | Speedup | Notes |
|---------------|---------------|---------|-------|
| Pragma SIMD   | ~15-20 min    | 2-3x    | CPU vectorization |
| Metal GPU     | ~2-3 min      | **8-10x** | GPU parallel compute |

### First Run vs Subsequent Runs

**First run** (Metal GPU):
- Metal shader compilation: ~1-2 seconds
- GPU initialization: prints "Metal GPU initialized successfully"
- Total includes one-time setup overhead

**Subsequent runs**:
- Metal objects cached in memory
- Faster initialization
- More consistent timings

## Output Files

After running `./build_all.sh compare`:

```
pragma_timing.log    # Full output from pragma SIMD run
metal_timing.log     # Full output from Metal GPU run
data/*.bin          # Binary simulation results
```

## Reading the Logs

### Timing format (macOS time -l)
```
       15.23 real         12.45 user         1.23 sys
  123456789  maximum resident set size
         0  average shared memory size
```

- **real**: Total elapsed time (wall clock)
- **user**: CPU time spent in your code
- **sys**: CPU time spent in system calls
- **maximum resident set size**: Peak memory in bytes

### Speedup Calculation

```
Speedup = (Pragma SIMD real time) / (Metal GPU real time)
```

Example:
```
Pragma: 900 seconds (15 min)
Metal:  120 seconds (2 min)
Speedup: 900/120 = 7.5x
```

## Performance Tips

### For Best Metal GPU Performance

1. **Grid size matters**: Larger grids see bigger speedup
   - Small (50×50×100): ~3-4x speedup
   - Medium (50×50×800): ~5-7x speedup
   - Large (50×50×1600): ~8-10x speedup

2. **First run**: Expect ~1-2s overhead for Metal initialization

3. **Binary I/O**: Using `.bin` instead of `.dat` saves significant time
   - Text I/O: ~45 seconds write time
   - Binary I/O: ~0.8 seconds write time

### For Best Pragma SIMD Performance

1. **Thread count**: Set in `config/simulation_parameters.h`
   ```cpp
   int num_threads = 6;  // Match your CPU core count
   ```

2. **Check OpenMP**: Ensure OpenMP is enabled
   ```bash
   # Should see parallel execution
   OMP_NUM_THREADS=6 ./main_pragma.out
   ```

## Troubleshooting

### Metal GPU not working?

Check for this message in output:
```
Metal GPU initialized successfully
```

If you see:
```
Metal initialization failed, falling back to CPU version
```

Then Metal GPU is not working. The code will use CPU fallback automatically.

### Pragma SIMD seems slow?

1. Check thread count: `echo $OMP_NUM_THREADS`
2. Verify SIMD compilation: `make clean && make` should show `-fopenmp`
3. Check CPU usage: should be near 100% × thread_count

### Memory issues?

For 50×50×1600 grid:
- Expected memory: ~3-4 GB
- Metal GPU: ~4-5 GB (includes GPU buffers)

Reduce grid size in config if needed.

## Comparing Different Configurations

Run multiple tests with different parameters:

```bash
# Test 1: Default configuration
./build_all.sh compare
mv pragma_timing.log results_default_pragma.log
mv metal_timing.log results_default_metal.log

# Test 2: Modify config (e.g., different grid size)
vim config/simulation_parameters.h
./build_all.sh compare
mv pragma_timing.log results_large_pragma.log
mv metal_timing.log results_large_metal.log
```

## Sample Output

```
════════════════════════════════════════════════════════
  Performance Comparison
════════════════════════════════════════════════════════

Running Pragma SIMD implementation...
──────────────────────────────────────────────────────
Metal GPU initialized successfully
[simulation output...]
      900.45 real       875.23 user        12.34 sys

Running Metal GPU implementation...
──────────────────────────────────────────────────────
Metal GPU initialized successfully
[simulation output...]
      120.67 real        45.12 user         8.23 sys

════════════════════════════════════════════════════════
  Results Summary
════════════════════════════════════════════════════════

Pragma SIMD timing:
      900.45 real       875.23 user        12.34 sys

Metal GPU timing:
      120.67 real        45.12 user         8.23 sys

Speedup: 7.5x

Full logs saved to:
  • pragma_timing.log
  • metal_timing.log
```

## Advanced: Profiling Individual Kernels

For more detailed profiling, use Instruments on macOS:

```bash
# Profile Metal GPU with Instruments
instruments -t "Time Profiler" ./main_metal.out

# Or use built-in Metal profiler
instruments -t "Metal System Trace" ./main_metal.out
```

This shows exactly where time is spent in the GPU kernel vs CPU overhead.
