#ifndef CONCENTRATION_FIELD_H
#define CONCENTRATION_FIELD_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

// CPU version (used as fallback)
void concentration_field(
  const double *c_n, double *c,
  int nx, int ny, int nz,
  double c_xx, double c_yy, double c_zz,
  double D_c, double Cxx, double Cyy, double Czz);

#ifdef __APPLE__
// Metal GPU accelerated version
void concentration_field_metal(
  const double *c_n, double *c,
  int nx, int ny, int nz,
  double c_xx, double c_yy, double c_zz,
  double D_c, double Cxx, double Cyy, double Czz);

// Cleanup function
void cleanup_metal_cf();
#endif

#endif  // CONCENTRATION_FIELD_H
