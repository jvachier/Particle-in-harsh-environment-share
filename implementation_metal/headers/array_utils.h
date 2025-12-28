#ifndef ARRAY_UTILS_H
#define ARRAY_UTILS_H

#include <stdio.h>

/**
 * @file array_utils.h
 * @brief Utilities for linear array indexing (replacing triple pointers)
 *
 * This header provides macros and utilities for accessing 3D data stored
 * in linear (1D) arrays. Linear storage provides:
 * - Contiguous memory (better cache locality)
 * - Zero-copy GPU transfers (same memory layout)
 * - Simpler memory management (single malloc/free)
 * - Better SIMD vectorization
 */

/**
 * @brief Convert 3D indices (i, j, k) to linear index
 *
 * For a 3D array of size [nx][ny][nz] stored linearly:
 * index = i * (ny * nz) + j * nz + k
 *
 * @param i First dimension index (0 to nx-1)
 * @param j Second dimension index (0 to ny-1)
 * @param k Third dimension index (0 to nz-1)
 * @param ny Size of second dimension
 * @param nz Size of third dimension
 * @return Linear index for accessing 1D array
 *
 * Example:
 *   Instead of: arr[i][j][k]
 *   Use:        arr[IDX3D(i, j, k, ny, nz)]
 */
#define IDX3D(i, j, k, ny, nz) ((i) * (ny) * (nz) + (j) * (nz) + (k))

/**
 * @brief Safe version with bounds checking (debug mode only)
 *
 * When DEBUG is defined, this macro checks bounds and prints error if out of range.
 * In release builds, it's identical to IDX3D for zero overhead.
 */
#ifdef DEBUG
#define IDX3D_SAFE(i, j, k, nx, ny, nz) \
    (((i) >= 0 && (i) < (nx) && (j) >= 0 && (j) < (ny) && (k) >= 0 && (k) < (nz)) \
     ? IDX3D(i, j, k, ny, nz) \
     : (fprintf(stderr, "ERROR: Index out of bounds [%d][%d][%d] (grid: %d×%d×%d)\n", \
                (i), (j), (k), (nx), (ny), (nz)), \
        IDX3D(i, j, k, ny, nz)))  // Return index anyway to avoid crash, but warn
#else
#define IDX3D_SAFE IDX3D
#endif

/**
 * @brief Calculate total size of 3D array
 */
#define ARRAY3D_SIZE(nx, ny, nz) ((size_t)(nx) * (ny) * (nz))

/**
 * @brief Calculate memory size in bytes for 3D double array
 */
#define ARRAY3D_BYTES(nx, ny, nz) (ARRAY3D_SIZE(nx, ny, nz) * sizeof(double))

/**
 * @brief Calculate memory size in megabytes for 3D double array
 */
#define ARRAY3D_MB(nx, ny, nz) (ARRAY3D_BYTES(nx, ny, nz) / (1024.0 * 1024.0))

#endif // ARRAY_UTILS_H
