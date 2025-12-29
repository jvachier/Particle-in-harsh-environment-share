#ifndef OLDTONEW_H
#define OLDTONEW_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

/**
 * Copy new values to old arrays for next iteration
 *
 * @param c_n Output: old concentration array
 * @param f_n Output: old density array
 * @param c   Input: new concentration array
 * @param f   Input: new density array
 * @param nx  Grid size in x direction
 * @param ny  Grid size in y direction
 * @param nz  Grid size in z direction
 */
void oldtonew(
    double *c_n, double *f_n,
    const double *c, const double *f,
    int nx, int ny, int nz);

#endif // OLDTONEW_H
