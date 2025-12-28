# Linear Array Refactoring Plan

## Objective

Refactor the entire codebase from triple pointer arrays (`double ***`) to linear arrays (`double *`) to eliminate GPU conversion overhead and achieve maximum performance.

---

## Current Data Structure

```cpp
// Triple pointer allocation (current)
double ***c = (double ***)malloc(nx * sizeof(double **));
for (int i = 0; i < nx; i++) {
    c[i] = (double **)malloc(ny * sizeof(double *));
    for (int j = 0; j < ny; j++) {
        c[i][j] = (double *)malloc(nz * sizeof(double));
    }
}

// Access: c[i][j][k]
```

**Problems:**
- Fragmented memory (poor cache locality)
- Requires expensive conversion for GPU
- Multiple malloc/free calls
- Pointer chasing overhead

---

## New Data Structure

```cpp
// Linear array allocation (new)
double *c = (double *)malloc(nx * ny * nz * sizeof(double));

// Access: c[IDX3D(i, j, k, ny, nz)]
// Where: IDX3D(i, j, k, ny, nz) = (i * ny * nz + j * nz + k)
```

**Benefits:**
- Contiguous memory (excellent cache locality)
- Zero conversion for GPU (same memory layout!)
- Single malloc/free
- Direct SIMD vectorization

---

## Refactoring Strategy

### Step 1: Create Index Macros

Add to a common header (e.g., `headers/array_utils.h`):

```cpp
// 3D array indexing for linear storage
#define IDX3D(i, j, k, ny, nz) ((i) * (ny) * (nz) + (j) * (nz) + (k))

// Safe bounds checking version (debug mode)
#ifdef DEBUG
#define IDX3D_SAFE(i, j, k, nx, ny, nz) \
    (((i) >= 0 && (i) < (nx) && (j) >= 0 && (j) < (ny) && (k) >= 0 && (k) < (nz)) \
     ? IDX3D(i, j, k, ny, nz) \
     : (fprintf(stderr, "Index out of bounds: [%d][%d][%d]\n", i, j, k), -1))
#else
#define IDX3D_SAFE IDX3D
#endif
```

### Step 2: Update Memory Allocation

**File:** `memory_allocation.cpp` / `memory_allocation.h`

```cpp
// Old signature:
double*** memory_allocation_3D(int nx, int ny, int nz);

// New signature:
double* memory_allocation_3D_linear(int nx, int ny, int nz);

// Implementation:
double* memory_allocation_3D_linear(int nx, int ny, int nz) {
    size_t total_size = nx * ny * nz;
    double *arr = (double *)malloc(total_size * sizeof(double));

    if (arr == NULL) {
        fprintf(stderr, "Error: Failed to allocate %zu MB\n",
                (total_size * sizeof(double)) / (1024 * 1024));
        exit(1);
    }

    // Initialize to zero
    memset(arr, 0, total_size * sizeof(double));

    return arr;
}
```

### Step 3: Update Deallocation

**File:** `delocate_memory.cpp` / `delocate_memory.h`

```cpp
// Old signature:
void delocate_memory_3D(double ***arr, int nx, int ny);

// New signature:
void delocate_memory_linear(double *arr);

// Implementation:
void delocate_memory_linear(double *arr) {
    if (arr != NULL) {
        free(arr);
    }
}
```

### Step 4: Update All Function Signatures

#### concentration_field.cpp / .h

```cpp
// Old:
void concentration_field(
    double ***c_n, double ***c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz);

// New:
void concentration_field(
    const double *c_n, double *c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz);
```

#### concentration_field_density.cpp / .h

```cpp
// Old:
void concentration_field_density(
    double ***c, double ***f, double ***f_n,
    double *advection, double *reaction, double *diffusion,
    int nx, int ny, int nz, ...);

// New:
void concentration_field_density(
    const double *c, double *f, const double *f_n,
    const double *advection, const double *reaction, const double *diffusion,
    int nx, int ny, int nz, ...);
```

#### oldtonew.cpp / .h

```cpp
// Old:
void oldtonew(double ***c_n, double ***f_n, double ***c, double ***f,
              int nx, int ny, int nz);

// New:
void oldtonew(double *c_n, double *f_n, const double *c, const double *f,
              int nx, int ny, int nz);

// Implementation becomes trivial:
void oldtonew(double *c_n, double *f_n, const double *c, const double *f,
              int nx, int ny, int nz) {
    size_t total_size = nx * ny * nz;
    memcpy(c_n, c, total_size * sizeof(double));
    memcpy(f_n, f, total_size * sizeof(double));
}
```

### Step 5: Update Kernel Implementations

#### concentration_field.cpp

```cpp
void concentration_field(
    const double *c_n, double *c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz) {

    #pragma omp parallel for collapse(2)
    for (int i = 1; i < nx - 1; i++) {
        for (int j = 1; j < ny - 1; j++) {
            #pragma omp simd
            for (int k = 1; k < nz - 1; k++) {
                int idx = IDX3D(i, j, k, ny, nz);
                int idx_im1 = IDX3D(i-1, j, k, ny, nz);
                int idx_ip1 = IDX3D(i+1, j, k, ny, nz);
                int idx_jm1 = IDX3D(i, j-1, k, ny, nz);
                int idx_jp1 = IDX3D(i, j+1, k, ny, nz);

                double c_n_curr = c_n[idx];
                double c_xx_val = c_n[idx_im1] - 2.0 * c_n_curr + c_n[idx_ip1];
                double c_yy_val = c_n[idx_jm1] - 2.0 * c_n_curr + c_n[idx_jp1];
                double c_zz_val = c_n[idx-1] - 2.0 * c_n_curr + c_n[idx+1];

                c[idx] = c_n_curr + D_c * (Cxx * c_xx_val + Cyy * c_yy_val + Czz * c_zz_val);
            }
        }
    }
}
```

#### concentration_field_density.cpp

Similar pattern - replace `arr[i][j][k]` with `arr[IDX3D(i, j, k, ny, nz)]`

### Step 6: Update Metal GPU Wrappers (ZERO-COPY!)

#### concentration_field_metal.mm

```cpp
void concentration_field_metal(
    const double *c_n, double *c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz) {

    if (!initialize_metal_cf()) {
        concentration_field(c_n, c, nx, ny, nz, c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);
        return;
    }

    @autoreleasepool {
        size_t totalSize = nx * ny * nz;
        size_t buffer_size = totalSize * sizeof(float);

        // Allocate GPU buffers only once
        if (g_cf_c_n_buffer == nil || g_cf_cached_buffer_size != buffer_size) {
            g_cf_c_n_buffer = [g_cf_device newBufferWithLength:buffer_size
                                                        options:MTLResourceStorageModeShared];
            g_cf_c_buffer = [g_cf_device newBufferWithLength:buffer_size
                                                      options:MTLResourceStorageModeShared];
            g_cf_cached_buffer_size = buffer_size;
        }

        // ZERO-COPY: Direct access to GPU memory
        float *c_n_gpu = (float*)[g_cf_c_n_buffer contents];
        float *c_gpu = (float*)[g_cf_c_buffer contents];

        // OPTIMIZED: Direct double→float conversion (no intermediate arrays!)
        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            c_n_gpu[i] = (float)c_n[i];
        }

        // ... run GPU kernel ...

        // OPTIMIZED: Direct float→double conversion
        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            c[i] = (double)c_gpu[i];
        }
    }
}
```

**Key benefit:** Conversion is now just type casting in a tight loop - no memory layout changes!

### Step 7: Update Initialization Functions

#### initialization.cpp

Replace all `arr[i][j][k]` with `arr[IDX3D(i, j, k, ny, nz)]`

### Step 8: Update I/O Functions

#### print_position.cpp

```cpp
void print_position(
    FILE *fc, FILE *ff,
    const double *c, const double *f,
    int nx, int ny, int nz,
    int n, double dt) {

    // Binary output remains efficient
    fwrite(&n, sizeof(int), 1, fc);
    fwrite(&n, sizeof(int), 1, ff);

    // Write entire array at once (even more efficient now!)
    fwrite(c, sizeof(double), nx * ny * nz, fc);
    fwrite(f, sizeof(double), nx * ny * nz, ff);
}
```

#### print_initial_position.cpp

```cpp
void print_initial_position(
    const double *c, const double *f,
    int nx, int ny, int nz,
    int sample_x, int sample_y, int sample_z) {

    int idx = IDX3D(sample_x, sample_y, sample_z, ny, nz);
    printf("%e\t%e\t%e\n", c[idx], f[idx], c[idx] * f[idx]);
}
```

### Step 9: Update main.cpp

```cpp
int main() {
    // ... parameter setup ...

    // OLD:
    // double ***c, ***c_n, ***f, ***f_n;
    // c = memory_allocation_3D(nx, ny, nz);
    // c_n = memory_allocation_3D(nx, ny, nz);
    // f = memory_allocation_3D(nx, ny, nz);
    // f_n = memory_allocation_3D(nx, ny, nz);

    // NEW:
    double *c, *c_n, *f, *f_n;
    c = memory_allocation_3D_linear(nx, ny, nz);
    c_n = memory_allocation_3D_linear(nx, ny, nz);
    f = memory_allocation_3D_linear(nx, ny, nz);
    f_n = memory_allocation_3D_linear(nx, ny, nz);

    // ... rest of code uses same function calls ...

    // OLD:
    // delocate_memory_3D(c, nx, ny);
    // delocate_memory_3D(c_n, nx, ny);
    // delocate_memory_3D(f, nx, ny);
    // delocate_memory_3D(f_n, nx, ny);

    // NEW:
    delocate_memory_linear(c);
    delocate_memory_linear(c_n);
    delocate_memory_linear(f);
    delocate_memory_linear(f_n);

    return 0;
}
```

---

## Implementation Order

1. ✅ Create `headers/array_utils.h` with index macros
2. ✅ Update `memory_allocation.cpp/h` - add linear functions
3. ✅ Update `delocate_memory.cpp/h` - add linear functions
4. ✅ Update `oldtonew.cpp/h` - simplest, good test case
5. ✅ Update `concentration_field.cpp/h` - medium complexity
6. ✅ Update `concentration_field_density.cpp/h` - most complex
7. ✅ Update `concentration_field_metal.mm` - zero-copy version
8. ✅ Update `concentration_field_density_metal.mm` - zero-copy version
9. ✅ Update `initialization.cpp/h`
10. ✅ Update `print_position.cpp/h`
11. ✅ Update `print_initial_position.cpp/h`
12. ✅ Update `main.cpp` - tie it all together
13. ✅ Test compilation
14. ✅ Validate numerical results (compare with old version)
15. ✅ Run performance tests

---

## Expected Performance

### Current (Triple Pointers)

- Pragma SIMD CPU: 145s
- Metal GPU: 178s (slower due to conversion overhead)

### After Refactor (Linear Arrays)

- Pragma SIMD CPU: ~130s (better cache locality, ~10% faster)
- Metal GPU: ~60-80s (zero-copy transfers, 2-3× faster than current CPU!)

**Target: Metal GPU becomes 2× faster than current best (145s → 60-80s)**

---

## Risk Mitigation

1. **Numerical validation:** Compare results with original triple pointer version
2. **Gradual testing:** Test each function individually before full integration
3. **Keep old code:** Maintain original triple pointer version for fallback
4. **Bounds checking:** Use `IDX3D_SAFE` macro during development/debugging

---

## Files to Modify (19 files)

### Headers (9 files)
1. headers/array_utils.h (NEW)
2. headers/concentration_field.h
3. headers/concentration_field_density.h
4. headers/oldtonew.h
5. headers/memory_allocation.h
6. headers/delocate_memory.h
7. headers/initialization.h
8. headers/print_position.h
9. headers/print_initial_position.h

### Implementation (10 files)
1. memory_allocation.cpp
2. delocate_memory.cpp
3. oldtonew.cpp
4. concentration_field.cpp
5. concentration_field_density.cpp
6. concentration_field_metal.mm
7. concentration_field_density_metal.mm
8. initialization.cpp
9. print_position.cpp
10. print_initial_position.cpp
11. main.cpp

---

## Success Criteria

- ✅ All code compiles without warnings
- ✅ Numerical results match original version (within floating-point tolerance)
- ✅ Memory usage remains similar (~150 MB)
- ✅ Pragma CPU version: ≤ 140s (should be slightly faster)
- ✅ **Metal GPU version: ≤ 80s (2× faster than current CPU!)**

Let's do this! 🚀
