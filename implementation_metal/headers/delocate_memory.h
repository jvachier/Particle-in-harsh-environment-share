#ifndef DELOCATE_MEMORY_H
#define DELOCATE_MEMORY_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

/**
 * Free a single linear 3D array
 *
 * @param arr Pointer to linear array to free
 */
void delocate_memory_linear(double *arr);

/**
 * Free all four main arrays (c, c_n, f, f_n)
 *
 * @param f    Pointer to f array
 * @param f_n  Pointer to f_n array
 * @param c    Pointer to c array
 * @param c_n  Pointer to c_n array
 * @param nx   Grid size (unused, kept for compatibility)
 * @param ny   Grid size (unused, kept for compatibility)
 * @param nz   Grid size (unused, kept for compatibility)
 */
void delocate_memory(
    double *f, double *f_n, double *c, double *c_n,
    int nx, int ny, int nz);

#endif // DELOCATE_MEMORY_H
