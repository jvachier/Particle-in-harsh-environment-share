#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

void print_initial_position(
  double ***f, double ***c,
  FILE *initialz, FILE *initialcz, FILE *initialx, FILE *initialcx,
  double dz, int nx, int nz);
