#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

void delocate_memory(
    double ***f, double ***f_n, double ***c, double ***c_n,
    int nx, int ny, int nz
);