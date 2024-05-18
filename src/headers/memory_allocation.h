#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

void memory_allocation_cn(
  double ***c_n, double ***c, double ***f_n, double ***f,
  int nx, int ny, int nz);
