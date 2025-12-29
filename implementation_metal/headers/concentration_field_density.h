#ifndef CONCENTRATION_FIELD_DENSITY_H
#define CONCENTRATION_FIELD_DENSITY_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

// Original CPU version (kept for fallback)
void concentration_field_density(
  const double *c, double *f, const double *f_n,
  const double *advection, const double *reaction, const double *diffusion,
  int nx, int ny, int nz,
  double dt, double beta, double c_x, double c_y, double c_z,
  double c_xx, double c_yy, double c_zz, double u_x, double u_y,
  double u_z, double u_xx, double u_yy, double u_zz, double Cx,
  double Cy, double Cz, double Cxx, double Cyy, double Czz,
  double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz);

#ifdef __APPLE__
// Metal GPU accelerated version
void concentration_field_density_metal(
  const double *c, double *f, const double *f_n,
  const double *advection, const double *reaction, const double *diffusion,
  int nx, int ny, int nz,
  double dt, double beta, double c_x, double c_y, double c_z,
  double c_xx, double c_yy, double c_zz, double u_x, double u_y,
  double u_z, double u_xx, double u_yy, double u_zz, double Cx,
  double Cy, double Cz, double Cxx, double Cyy, double Czz,
  double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz);

// Cleanup function
void cleanup_metal();
#endif

#endif  // CONCENTRATION_FIELD_DENSITY_H
