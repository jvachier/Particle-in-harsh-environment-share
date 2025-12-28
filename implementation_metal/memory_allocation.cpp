#include "headers/memory_allocation.h"
#include "headers/array_utils.h"
#include <cstring>  // for memset

using namespace std;

/**
 * Allocate a 3D array as a linear (contiguous) block of memory
 *
 * Benefits of linear storage:
 * - Single allocation (fast)
 * - Contiguous memory (excellent cache locality)
 * - Zero-copy GPU transfers
 * - Simple deallocation
 *
 * @param nx First dimension size
 * @param ny Second dimension size
 * @param nz Third dimension size
 * @return Pointer to allocated linear array, or NULL on failure
 */
double* memory_allocation_3D_linear(int nx, int ny, int nz) {
    size_t bytes = ARRAY3D_BYTES(nx, ny, nz);

    double *arr = (double *)malloc(bytes);

    if (arr == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate %.2f MB for %dx%dx%d array\n",
                ARRAY3D_MB(nx, ny, nz), nx, ny, nz);
        exit(1);
    }

    // Initialize to zero
    memset(arr, 0, bytes);

    return arr;
}

/**
 * Allocate multiple 3D arrays at once
 *
 * This is the main allocation function called from main.cpp
 *
 * @param c_n Pointer to store allocated c_n array
 * @param c   Pointer to store allocated c array
 * @param f_n Pointer to store allocated f_n array
 * @param f   Pointer to store allocated f array
 * @param nx  First dimension size
 * @param ny  Second dimension size
 * @param nz  Third dimension size
 */
void memory_allocation_cn(
    double **c_n_ptr, double **c_ptr,
    double **f_n_ptr, double **f_ptr,
    int nx, int ny, int nz) {

    printf("Allocating arrays: %dx%dx%d (%.2f MB each, %.2f MB total)\n",
           nx, ny, nz,
           ARRAY3D_MB(nx, ny, nz),
           4 * ARRAY3D_MB(nx, ny, nz));

    *c_n_ptr = memory_allocation_3D_linear(nx, ny, nz);
    *c_ptr   = memory_allocation_3D_linear(nx, ny, nz);
    *f_n_ptr = memory_allocation_3D_linear(nx, ny, nz);
    *f_ptr   = memory_allocation_3D_linear(nx, ny, nz);

    printf("Memory allocation successful\n");
}
