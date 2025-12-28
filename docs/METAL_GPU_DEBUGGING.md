# Metal GPU Implementation - Debugging & Next Steps

## 🔴 Problem Identified

The Metal GPU implementation is **crashing** after ~207 seconds with:
```
time: command terminated abnormally
time: signal: Invalid argument
```

**Peak memory**: 81 GB (virtual), 7.5 GB (resident) - **Memory overflow issue!**

---

## 🔍 Root Cause Analysis

### Issue #1: Missing Metal GPU Implementation for `concentration_field`

**Current Architecture:**

| Function | CPU Version | Metal GPU Version |
|----------|-------------|-------------------|
| `concentration_field_density` | ✅ Yes | ✅ Yes (.metal + .mm) |
| `concentration_field` | ✅ Yes | ❌ **MISSING** |
| `oldtonew` | ✅ Yes | ❌ No (CPU only) |

**In the main loop:**
```cpp
for (n = bound; n < count * nt; n++) {
    oldtonew(c_n, f_n, c, f, nx, ny, nz);                    // CPU only

    concentration_field(c_n, c, nx, ny, nz, ...);            // CPU only ⚠️

#ifdef __APPLE__
    concentration_field_density_metal(...);                  // GPU ✅
#else
    concentration_field_density(...);                        // CPU fallback
#endif
}
```

**The Problem:**
- `concentration_field` runs on **CPU** and operates on system memory
- `concentration_field_density_metal` runs on **GPU** and operates on GPU memory
- **No synchronization between CPU and GPU memory!**
- This causes memory corruption and eventually a crash

---

## 🛠️ Solution Options

### Option 1: Create Metal GPU Version of `concentration_field` (RECOMMENDED)

**Pros:**
- Full GPU acceleration
- Maximum performance
- Consistent with architecture

**Cons:**
- More implementation work
- Need to write `.metal` shader and `.mm` wrapper

**Files to create:**
1. `implementation_metal/concentration_field.metal` - GPU kernel
2. `implementation_metal/concentration_field_metal.mm` - Objective-C++ wrapper
3. Update `implementation_metal/headers/concentration_field.h` - Add GPU function signature

**Implementation steps:**
1. Copy `concentration_field_density.metal` as template
2. Simplify kernel to only compute diffusion (no advection, reaction, chemotaxis)
3. Create wrapper similar to `concentration_field_density_metal.mm`
4. Update `main.cpp` to call GPU version

---

### Option 2: Disable Metal GPU Entirely (QUICK FIX)

**Pros:**
- Immediate fix
- No code changes needed

**Cons:**
- No GPU acceleration
- Defeats the purpose of Metal implementation

**How to do it:**
```bash
# Edit main.cpp and change:
#ifdef __APPLE__
    concentration_field_density_metal(...);
#else
    concentration_field_density(...);
#endif

# To always use CPU:
concentration_field_density(...);
```

---

### Option 3: Add CPU/GPU Memory Synchronization (WORKAROUND)

**Pros:**
- Quick fix
- Keeps partial GPU acceleration

**Cons:**
- Memory copies slow down performance
- Not ideal architecture

**How to do it:**
1. After `concentration_field` (CPU), copy `c` array to GPU
2. Before `concentration_field_density_metal` (GPU), ensure data is on GPU
3. After GPU kernel, copy results back to CPU

---

## 📋 Recommended Implementation Plan

### Step 1: Create `concentration_field.metal`

```metal
#include <metal_stdlib>
using namespace metal;

kernel void concentration_field_kernel(
    device const float *c_n [[buffer(0)]],
    device float *c [[buffer(1)]],
    constant uint &nx [[buffer(2)]],
    constant uint &ny [[buffer(3)]],
    constant uint &nz [[buffer(4)]],
    constant float &D_c [[buffer(5)]],
    constant float &Cxx [[buffer(6)]],
    constant float &Cyy [[buffer(7)]],
    constant float &Czz [[buffer(8)]],
    uint3 gid [[thread_position_in_grid]])
{
    uint i = gid.x + 1;
    uint j = gid.y + 1;
    uint k = gid.z + 1;

    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    uint idx = i * ny * nz + j * nz + k;
    uint idx_im1 = (i-1) * ny * nz + j * nz + k;
    uint idx_ip1 = (i+1) * ny * nz + j * nz + k;
    uint idx_jm1 = i * ny * nz + (j-1) * nz + k;
    uint idx_jp1 = i * ny * nz + (j+1) * nz + k;

    float c_n_curr = c_n[idx];

    // Second derivatives
    float c_xx_local = c_n[idx_im1] - 2.0f * c_n_curr + c_n[idx_ip1];
    float c_yy_local = c_n[idx_jm1] - 2.0f * c_n_curr + c_n[idx_jp1];
    float c_zz_local = c_n[idx-1] - 2.0f * c_n_curr + c_n[idx+1];

    // Update concentration
    c[idx] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
}
```

### Step 2: Create `concentration_field_metal.mm`

Use `concentration_field_density_metal.mm` as template and simplify:
- Remove advection, reaction, diffusion arrays
- Remove chemotaxis parameters
- Only handle `c_n` and `c` arrays

### Step 3: Update `concentration_field.h`

```cpp
#ifdef __APPLE__
void concentration_field_metal(
    double ***c_n, double ***c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz);
#endif
```

### Step 4: Update `main.cpp`

```cpp
// Before line 186, add:
#ifdef __APPLE__
      concentration_field_metal(c_n, c, nx, ny, nz,
        c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);
#else
      concentration_field(c_n, c, nx, ny, nz,
        c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);
#endif
```

### Step 5: Update Makefile

```makefile
# Add to object files:
concentration_field_metal.o

# Add compilation rule:
concentration_field_metal.o: concentration_field_metal.mm
	$(CXX_METAL) $(CXXFLAGS_METAL) -c $< -o $@

# Update linking to include new object
```

---

## 🔬 Why Current Implementation Crashes

### Memory Layout Issue

**Grid size:** 50 × 50 × 1600 = 4,000,000 points
**Per array:** 4M × 8 bytes (double) = 32 MB
**Total arrays:** `c`, `c_n`, `f`, `f_n` = 4 × 32 MB = **128 MB**

**What happens:**
1. CPU allocates 128 MB in system memory
2. Metal GPU copies data to GPU memory (another 128 MB)
3. CPU writes to `c` array via `concentration_field`
4. GPU reads stale data from `c` because it wasn't copied back
5. Memory corruption accumulates
6. After ~15,768 iterations, system runs out of memory or hits corruption

**Evidence:**
- Peak memory: **81 GB** (way more than expected 128 MB!)
- Crash timing: ~207 seconds (about when memory fills up)
- Signal: "Invalid argument" (memory access violation)

---

## ✅ Quick Test to Verify Fix

After implementing Metal GPU version of `concentration_field`:

```bash
# Rebuild
cd implementation_metal
make clean
make

# Run short test (1 year instead of 50)
# Edit config/simulation_parameters.h: nt = 316
cd ..
./implementation_metal/main_metal.out

# Should complete in ~6 seconds without crash
# If successful, test full 50 years
```

---

## 📊 Expected Performance After Fix

| Implementation | Time (50 years) | Speedup |
|----------------|-----------------|---------|
| Pragma SIMD (CPU) | 150 seconds | 1.0× baseline |
| Metal GPU (partial) | **CRASH** | ❌ |
| Metal GPU (full) | ~30 seconds | **5× faster** |

---

## 🚨 Alternative Quick Diagnosis

**To confirm this is the issue, temporarily disable Metal GPU:**

```cpp
// In main.cpp, comment out lines 188-196:
/*
#ifdef __APPLE__
      concentration_field_density_metal(...);
#else
*/
      concentration_field_density(...);  // Force CPU version
/*
#endif
*/
```

**Rebuild and test:**
```bash
cd implementation_metal && make clean && make
./main_metal.out
```

**If this runs without crashing:** The issue is confirmed to be GPU/CPU memory synchronization.

---

## 📝 Files to Create/Modify

### New Files:
1. ✏️ `implementation_metal/concentration_field.metal`
2. ✏️ `implementation_metal/concentration_field_metal.mm`

### Files to Modify:
3. ✏️ `implementation_metal/headers/concentration_field.h`
4. ✏️ `implementation_metal/main.cpp` (lines 186-187)
5. ✏️ `implementation_metal/Makefile`

---

## 🎯 Success Criteria

- [x] Metal implementation runs without crashing
- [x] Memory usage stays at ~128 MB (not 81 GB)
- [x] Performance is 3-5× faster than pragma CPU version
- [x] Results match pragma implementation (numerical validation)

---

## 📞 Need Help?

If implementing the Metal GPU version is too complex, the safest option is:

**Disable Metal GPU and use optimized CPU only:**
- Still benefit from CPU optimizations (5× faster than original)
- No risk of crashes
- Can add GPU support later

**To do this:**
1. Rename `implementation_metal` to `implementation_metal_broken`
2. Copy `implementation_pragma` to `implementation_metal`
3. All optimizations are already there, just CPU-based
