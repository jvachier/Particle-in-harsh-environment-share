#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

void initialization(
  double ***f, double ***c,
  int nx, int ny, int nz,
  double z_0, double dx, double dy, double dz, double norm1, double norm2);
