#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>



void concentration_field_density(
    double ***c, double ***f, double ***f_n, 
    double *advection, double *reaction, double *diffusion, 
    int nx, int ny, int nz, 
    double dt, double beta, double c_x, double c_y, double c_z, 
    double c_xx, double c_yy, double c_zz, double u_x, double u_y, 
    double u_z, double u_xx, double u_yy, double u_zz, double Cx, 
    double Cy, double Cz, double Cxx, double Cyy, double Czz, 
    double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz
);