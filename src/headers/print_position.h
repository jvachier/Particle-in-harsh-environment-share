#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

void print_position(
  double ***f, double ***c,
  FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
  double dz, int nx, int nz);
