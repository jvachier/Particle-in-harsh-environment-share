#include "headers/print_position.h"
#include "headers/array_utils.h"
#include <stdexcept>
#include <cstdio>

using namespace std;

// Binary output version - much faster and smaller files than text format
int print_position(
    const double *f, const double *c,
    FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
    double dz, int nx, int ny, int nz,
    int sample_x, int sample_y, int sample_z) {
  // Input validation
  if (!f || !c) {
    fprintf(stderr, "Error: NULL array pointer\n");
    return -1;
  }
  if (!fpz || !fcz || !fpx || !fcx) {
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
    double f_val = f[IDX3D(sample_x, sample_y, k, ny, nz)];
    double c_val = c[IDX3D(sample_x, sample_y, k, ny, nz)];

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
    double f_val = f[IDX3D(i, sample_y, sample_z, ny, nz)];
    double c_val = c[IDX3D(i, sample_y, sample_z, ny, nz)];

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

  return 0;  // Success
}
