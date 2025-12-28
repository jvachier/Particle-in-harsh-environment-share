#include "headers/initialization.h"
#include "headers/array_utils.h"

using namespace std;

void initialization(
  double *f, double *c,
  int nx, int ny, int nz,
  double z_0, double dx, double dy, double dz, double norm1, double norm2) {
#pragma omp parallel for simd collapse(3)
    for (int i = 0; i < nx; i++) {
      for (int j = 0; j < ny; j++) {
        for (int k = 0; k < nz; k++) {
          f[IDX3D(i, j, k, ny, nz)] = exp(-((k * dz - z_0) * (k * dz - z_0)) / 20.0) \
            * exp(-(i * dx - 5.0) * (i * dx - 5.0)) \
            * exp(-(i * dy - 5.0) * (i * dy - 5.0)) * norm1;
          c[IDX3D(i, j, k, ny, nz)] = exp(-(k * dz - 55.0) * (k * dz - 55.0)) \
            * exp(-(i * dx - 5.0) * (i * dx - 5.0)) \
            * exp(-(j * dy - 5.0) * (j * dy - 5.0)) * norm2;
          }
        }
    }
}
