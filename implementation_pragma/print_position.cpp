#include "headers/print_position.h"
#include <stdexcept>
#include <cstdio>

using namespace std;

// Binary output version - much faster and smaller files than text format
int print_position(
    double ***f, double ***c,
    FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
    FILE *fpy, FILE *fcy,
    double dz, int nx, int ny, int nz,
    int sample_x, int sample_y, int sample_z) {
  // Input validation
  if (!f || !c) {
    fprintf(stderr, "Error: NULL array pointer\n");
    return -1;
  }
  if (!fpz || !fcz || !fpx || !fcx || !fpy || !fcy) {
    fprintf(stderr, "Error: NULL file pointer\n");
    return -1;
  }
  if (sample_x < 0 || sample_x >= nx ||
      sample_y < 0 || sample_y >= ny ||
      sample_z < 0 || sample_z >= nz) {
    fprintf(stderr, "Error: Sample point out of bounds\n");
    return -1;
  }

  // Write z-direction profiles at fixed (x,y) = (sample_x, sample_y)
  // Binary format: alternating position and value pairs
  for (int k = 0; k < nz; k++) {
    double z_pos = k * dz;
    double f_val = f[sample_x][sample_y][k];
    double c_val = c[sample_x][sample_y][k];

    if (fwrite(&z_pos, sizeof(double), 1, fpz) != 1 ||
        fwrite(&f_val, sizeof(double), 1, fpz) != 1) {
      fprintf(stderr, "Error: Failed to write binary z-profile (f) at k=%d\n", k);
      return -1;
    }
    if (fwrite(&z_pos, sizeof(double), 1, fcz) != 1 ||
        fwrite(&c_val, sizeof(double), 1, fcz) != 1) {
      fprintf(stderr, "Error: Failed to write binary z-profile (c) at k=%d\n", k);
      return -1;
    }
  }

  // Write x-direction profiles at fixed (y,z) = (sample_y, sample_z)
  for (int i = 0; i < nx; i++) {
    double x_pos = i * dz;
    double f_val = f[i][sample_y][sample_z];
    double c_val = c[i][sample_y][sample_z];

    if (fwrite(&x_pos, sizeof(double), 1, fpx) != 1 ||
        fwrite(&f_val, sizeof(double), 1, fpx) != 1) {
      fprintf(stderr, "Error: Failed to write binary x-profile (f) at i=%d\n", i);
      return -1;
    }
    if (fwrite(&x_pos, sizeof(double), 1, fcx) != 1 ||
        fwrite(&c_val, sizeof(double), 1, fcx) != 1) {
      fprintf(stderr, "Error: Failed to write binary x-profile (c) at i=%d\n", i);
      return -1;
    }
  }

  // Write y-direction profiles at fixed (x,z) = (sample_x, sample_z)
  for (int j = 0; j < ny; j++) {
    double y_pos = j * dz;
    double f_val = f[sample_x][j][sample_z];
    double c_val = c[sample_x][j][sample_z];

    if (fwrite(&y_pos, sizeof(double), 1, fpy) != 1 ||
        fwrite(&f_val, sizeof(double), 1, fpy) != 1) {
      fprintf(stderr, "Error: Failed to write binary y-profile (f) at j=%d\n", j);
      return -1;
    }
    if (fwrite(&y_pos, sizeof(double), 1, fcy) != 1 ||
        fwrite(&c_val, sizeof(double), 1, fcy) != 1) {
      fprintf(stderr, "Error: Failed to write binary y-profile (c) at j=%d\n", j);
      return -1;
    }
  }

  return 0;  // Success
}
