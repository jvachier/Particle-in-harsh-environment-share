#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>


void oldtonew(
  double ***c_n, double ***f_n, double ***c, double ***f,
  int nx, int ny, int nz);
