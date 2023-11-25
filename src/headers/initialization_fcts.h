#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>



void initialization_fcts(
    double *advection, double *reaction, double *diffusion, 
    int nz, 
    double dz, double AA, double BB, double D_a
);