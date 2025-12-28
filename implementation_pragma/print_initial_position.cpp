#include "headers/print_initial_position.h"
#include <cstdio>

using namespace std;

// Binary output version - matches print_position.cpp format
void print_initial_position(
  double ***f, double ***c,
  FILE *initialz, FILE *initialcz, FILE *initialx, FILE *initialcx,
  double dz, int nx, int ny, int nz,
  int sample_x, int sample_y, int sample_z) {

  // Write z-direction initial profiles (binary format)
  for (int k = 0; k < nz; k++) {
    double z_pos = k * dz;
    double f_val = f[sample_x][sample_y][k];
    double c_val = c[sample_x][sample_y][k];

    // Write binary: position, value pairs
    fwrite(&z_pos, sizeof(double), 1, initialz);
    fwrite(&f_val, sizeof(double), 1, initialz);

    fwrite(&z_pos, sizeof(double), 1, initialcz);
    fwrite(&c_val, sizeof(double), 1, initialcz);
  }

  // Write x-direction initial profiles (binary format)
  for (int i = 0; i < nx; i++) {
    double x_pos = i * dz;
    double f_val = f[i][sample_y][sample_z];
    double c_val = c[i][sample_y][sample_z];

    fwrite(&x_pos, sizeof(double), 1, initialx);
    fwrite(&f_val, sizeof(double), 1, initialx);

    fwrite(&x_pos, sizeof(double), 1, initialcx);
    fwrite(&c_val, sizeof(double), 1, initialcx);
  }
}
