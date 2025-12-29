#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

void print_initial_position(
  const double *f, const double *c,
  FILE *initialz, FILE *initialcz, FILE *initialx, FILE *initialcx,
  FILE *initialy, FILE *initialcy,
  double dz, int nx, int ny, int nz,
  int sample_x, int sample_y, int sample_z);
