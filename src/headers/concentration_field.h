#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>


void concentration_field(
    double ***c_n, double ***c, 
    int nx, int ny, int nz, 
    double c_xx, double c_yy, double c_zz, double D_c, double Cxx, double Cyy, double Czz
);
