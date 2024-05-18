#include "headers/concentration_field.h"

using namespace std;

void concentration_field(
  double ***c_n, double ***c,
  int nx, int ny, int nz,
  double c_xx, double c_yy, double c_zz,
  double D_c, double Cxx, double Cyy, double Czz) {
#pragma omp parallel for simd collapse(3)
    for (int i = 1; i < nx - 1; i++) {
      for (int j = 1; j < ny - 1; j++) {
        for (int k = 1; k < nz - 1; k++) {
          c_xx = c_n[i - 1][j][k] - 2.0 * c_n[i][j][k] + c_n[i + 1][j][k];
          c_yy = c_n[i][j - 1][k] - 2.0 * c_n[i][j][k] + c_n[i][j + 1][k];
          c_zz = c_n[i][j][k - 1] - 2.0 * c_n[i][j][k] + c_n[i][j][k + 1];
          // CONCENTRATION
          c[i][j][k] = c_n[i][j][k] + \
            D_c * (Cxx * c_xx + Cyy * c_yy + Czz * c_zz);
        }
      }
    }
}
