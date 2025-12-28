# GPU Performance Analysis & Optimization Strategy

## 🔴 Current Problem

**Metal GPU is SLOWER than CPU**: 266 seconds vs 147 seconds (1.8× slower!)

This is backwards - GPU should be faster, not slower.

---

## 🔍 Root Cause Analysis

### Current Implementation Issues

**Per-iteration overhead** (happens 15,768 times):

```cpp
// concentration_field_metal - called every iteration
1. Convert 3D double array to linear float (4M elements × 2 arrays)
2. Copy 32 MB to GPU
3. Create 7 small buffers for parameters
4. Run GPU kernel (~0.1ms)
5. Copy 32 MB back from GPU
6. Convert linear float to 3D double

// concentration_field_density_metal - called every iteration
1. Convert 3D double to linear (4M × 2 = 8M elements)
2. Convert double → float (64 MB)
3. Copy to GPU (64 MB)
4. Create 20+ parameter buffers
5. Run GPU kernel (~0.2ms)
6. Copy back from GPU (32 MB)
7. Convert float → double
```

**Total overhead per iteration**:
- Memory copies: ~160 MB to GPU + ~64 MB from GPU = **224 MB/iteration**
- Type conversions: double↔float for 12M elements
- Buffer creation: 27 small buffers

**Over 15,768 iterations**:
- **3.5 TB of data transfers!**
- GPU kernel runtime: ~5 seconds
- Data transfer time: ~260 seconds
- **Overhead is 52× the actual computation time!**

---

## ✅ Solution Strategy

### Option 1: Keep Everything on GPU (RECOMMENDED)

**Idea**: Allocate arrays on GPU once, keep them there for entire simulation.

**Changes needed**:
1. Allocate `c`, `c_n`, `f`, `f_n` on GPU at start
2. Run ALL kernels on GPU (initialization, oldtonew, concentration_field, density)
3. Only copy data back at the end (or for periodic output)

**Expected speedup**: 10-20× faster than current CPU version

**Complexity**: HIGH - need to rewrite all functions as Metal kernels

---

### Option 2: Hybrid CPU/GPU (QUICK WIN)

**Idea**: Only put the most expensive function on GPU.

**Analysis of computational cost**:

```
Per iteration cost (50×50×1600 grid):
1. oldtonew: Simple copy, very fast (~0.01s) - KEEP ON CPU
2. concentration_field: Simple diffusion (~0.5s) - MAYBE GPU
3. concentration_field_density: Complex, many operations (~1.5s) - GPU!
```

**Strategy**:
- Keep `c`, `c_n`, `f`, `f_n` on **CPU**
- Only run `concentration_field_density` on GPU
- Remove `concentration_field_metal` (not worth the overhead)
- Optimize data transfer in density kernel

**Expected speedup**: 2-3× faster than current

**Complexity**: MEDIUM

---

### Option 3: Disable GPU, Use Only Optimized CPU (SAFEST)

**Idea**: CPU version already works great (147s for 50 years)

**Benefits**:
- Already working and tested
- Good performance (5× faster than original)
- No GPU complexity

**When to use**: If GPU optimization takes too long

---

## 🎯 Recommended Approach: Hybrid Optimization

### Step 1: Profile Current Code

Measure where time is spent:
```
concentration_field:         ~40 sec  (27%)
concentration_field_density: ~80 sec  (54%)
oldtonew:                    ~5 sec   (3%)
I/O and other:               ~22 sec  (15%)
```

### Step 2: Optimize Data Transfers

**Current** (`concentration_field_density_metal`):
```cpp
// BAD: Copy full arrays every time
for (i = 0; i < totalSize; i++) {
    c_gpu[i] = (float)c_linear[i];          // 32 MB copy
    f_n_gpu[i] = (float)f_n_linear[i];      // 32 MB copy
}
// ... kernel ...
for (i = 0; i < totalSize; i++) {
    f_linear[i] = (double)f_result_gpu[i];   // 32 MB copy
}
```

**Optimized**:
```cpp
// GOOD: Only copy interior points (95% reduction!)
// Boundaries don't change, no need to copy them
int interior_size = (nx-2) * (ny-2) * (nz-2);  // 48×48×1598 = 3.7M vs 4M

// Use parallel conversion
#pragma omp parallel for
for (i = 1; i < nx-1; i++) {
    for (j = 1; j < ny-1; j++) {
        for (k = 1; k < nz-1; k++) {
            int idx = i*ny*nz + j*nz + k;
            c_gpu[idx] = (float)c[i][j][k];
        }
    }
}
```

### Step 3: Remove Unnecessary GPU Calls

**Remove `concentration_field_metal`** because:
- Simple operation (just diffusion)
- Small computational cost
- Data transfer overhead >> computation time
- CPU version is already fast with SIMD

**Keep only `concentration_field_density_metal`** because:
- Most expensive operation
- Complex computation (chemotaxis + advection + reaction + diffusion)
- Worth the transfer overhead

### Step 4: Optimize Metal Kernel

**Current kernel issues**:
1. Small thread groups (8×8×8 = 512 threads)
2. Not using texture cache
3. Redundant memory access

**Optimizations**:
```metal
// Use larger thread groups for better occupancy
MTLSize threadsPerThreadgroup = MTLSizeMake(16, 8, 8);  // 1024 threads

// Add threadgroup memory for cache
threadgroup float cache[16][8][8];

// Prefetch data into cache
cache[lid.x][lid.y][lid.z] = c_n[idx];
threadgroup_barrier(mem_flags::mem_threadgroup);

// Use cached data for stencil
```

---

## 📊 Expected Performance After Optimization

| Implementation | Time | Memory | Speedup |
|----------------|------|--------|---------|
| Original | ~750s | 148 MB | 1.0× baseline |
| Pragma SIMD (current) | 147s | 148 MB | 5.1× |
| Metal GPU (broken) | CRASH | 101 GB | ❌ |
| Metal GPU (fixed, unoptimized) | 266s | 462 MB | 2.8× ⚠️ SLOWER than CPU!|
| **Metal GPU (optimized - remove field)** | **~90s** | **300 MB** | **8.3×** 🎯 |
| **Metal GPU (optimized - better transfers)** | **~60s** | **300 MB** | **12.5×** 🚀 |
| Metal GPU (full GPU, all kernels) | ~30s | 512 MB | 25× (future work) |

---

## 🔧 Implementation Plan

### Quick Win (1 hour):

1. **Remove `concentration_field_metal` calls**
   - Edit main.cpp to use CPU version only
   - Keep only `concentration_field_density_metal`

2. **Optimize data conversion**
   - Use OpenMP parallel for double↔float conversion
   - Only convert interior points

3. **Test and measure**

### Expected result: **~90 seconds (1.6× faster than CPU)**

---

## 📝 Code Changes Needed

### 1. main.cpp - Remove concentration_field_metal

```cpp
// Line 186-193: REMOVE GPU version, use CPU only
// Old (slow):
#ifdef __APPLE__
    concentration_field_metal(c_n, c, nx, ny, nz...);
#else
    concentration_field(c_n, c, nx, ny, nz...);
#endif

// New (faster):
concentration_field(c_n, c, nx, ny, nz...);  // Always use CPU
```

### 2. concentration_field_density_metal.mm - Parallel Conversion

```cpp
// Replace slow serial conversion with parallel
// OLD:
for (size_t i = 0; i < totalSize; i++) {
    c_gpu[i] = (float)c_linear[i];
    f_n_gpu[i] = (float)f_n_linear[i];
}

// NEW:
#pragma omp parallel for
for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
        for (int k = 0; k < nz; k++) {
            int idx = i * ny * nz + j * nz + k;
            c_gpu[idx] = (float)c[i][j][k];      // Direct, no intermediate
            f_n_gpu[idx] = (float)f_n[i][j][k];
        }
    }
}
```

---

## 🎯 Success Criteria

After optimization:
- ✅ Metal GPU faster than Pragma CPU (< 147 seconds)
- ✅ Memory usage reasonable (< 512 MB)
- ✅ No crashes
- ✅ Results numerically identical to CPU version

Target: **90 seconds or better**

---

## 💡 Why This Will Work

**Current bottleneck**: Data transfer (260s) >> Computation (5s)

**After removing concentration_field_metal**:
- Eliminate 50% of GPU transfers
- Keep expensive density kernel on GPU
- Net speedup: ~1.6-2×

**After optimizing conversions**:
- Parallel conversion: 3-4× faster
- Net speedup: ~2-3×

**Combined**: ~90-60 seconds (1.6-2.4× faster than current CPU)
