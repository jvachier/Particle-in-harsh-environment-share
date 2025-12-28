#ifndef PRINT_POSITION_H
#define PRINT_POSITION_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

// Original function signature (now improved with error checking)
int print_position(
  const double *f, const double *c,
  FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
  double dz, int nx, int ny, int nz,
  int sample_x, int sample_y, int sample_z);

// SIMD-optimized version
int print_position_simd(
  const double *f, const double *c,
  FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
  double dz, int nx, int ny, int nz,
  int sample_x, int sample_y, int sample_z);

#ifdef __APPLE__
// Metal GPU-accelerated version
int print_position_metal(
  const double *f, const double *c,
  FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
  double dz, int nx, int ny, int nz,
  int sample_x, int sample_y, int sample_z);
#endif

#endif  // PRINT_POSITION_H
