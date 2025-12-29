#ifndef MEMORY_ALLOCATION_H
#define MEMORY_ALLOCATION_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

/**
 * Allocate a single 3D array as linear storage
 *
 * @param nx First dimension size
 * @param ny Second dimension size
 * @param nz Third dimension size
 * @return Pointer to allocated linear array
 */
double* memory_allocation_3D_linear(int nx, int ny, int nz);

/**
 * Allocate all four main arrays (c, c_n, f, f_n) as linear storage
 *
 * @param c_n_ptr Pointer to store allocated c_n array
 * @param c_ptr   Pointer to store allocated c array
 * @param f_n_ptr Pointer to store allocated f_n array
 * @param f_ptr   Pointer to store allocated f array
 * @param nx      First dimension size
 * @param ny      Second dimension size
 * @param nz      Third dimension size
 */
void memory_allocation_cn(
    double **c_n_ptr, double **c_ptr,
    double **f_n_ptr, double **f_ptr,
    int nx, int ny, int nz);

#endif // MEMORY_ALLOCATION_H
